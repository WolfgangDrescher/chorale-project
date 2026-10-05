<script setup>
import { useClipboard } from '@vueuse/core';

const { t } = useI18n();

const localePath = useLocalePath();

useHead({
    title: t('segmentAnalysis'),
});

// Whole quarter notes, which is all --length takes. Anything shorter than 2 makes the query a
// single note, anything past 8 rarely survives a phrase ending (no segment reaches across a
// fermata), so the ends of this list are already where the tool stops being interesting.
const SEGMENT_LENGTHS = [2, 3, 4, 6, 8];

// The options sent along with the score, rendered as one form field each. To add one: add an
// entry here (type 'select' with `items`, or 'switch'), its two translations, and handle its
// key in server/api/chorale-segment.js.
const CHECK_OPTIONS = [
    {
        key: 'length',
        type: 'select',
        items: SEGMENT_LENGTHS,
        default: 4,
        label: 'segmentLength',
        description: 'segmentLengthDescription',
    },
];

const DEMO_CHORALE_ID = 'chor029';

// The demo score is a development aid, not part of the page.
const isDev = import.meta.dev;

// Mirrors the endpoint's cap on "data", so an oversized file is refused before it travels.
const MAX_FILE_SIZE = 2 * 1024 * 1024;

// Extensions only: Nuxt UI turns MIME types in `accept` into a filter for dropped files, and a
// dropped .krn file has none, so every drop would be refused.
const UPLOAD_ACCEPT = '.krn,.kern,.musicxml,.xml';

const KERN_EXTENSIONS = ['.krn', '.kern'];
const MUSICXML_EXTENSIONS = ['.musicxml', '.xml'];

function useSegmentAnalysis() {
    const file = ref(null);
    const options = reactive(Object.fromEntries(CHECK_OPTIONS.map((option) => [option.key, option.default])));
    const pending = ref(false);
    const progress = ref(null);
    const error = ref(null);
    const result = ref(null); // { inputFormat, layout, kern, segments, durationMs }

    // 1-based, so it doubles as UPagination's page with one segment per page.
    const position = ref(1);

    const segments = computed(() => result.value?.segments ?? []);
    const activeSegment = computed(() => segments.value[position.value - 1] ?? null);

    async function analyze(data) {
        pending.value = true;
        progress.value = null;
        error.value = null;
        result.value = null;
        position.value = 1;
        try {
            // The segments arrive with their stats already on them: the endpoint runs
            // chorale-segment --stats, which counts every segment's query against the corpus
            // in the same run.
            result.value = await fetchWithProgress('/api/chorale-segment', {
                body: { data, ...options },
                onProgress: (event) => {
                    progress.value = { ...progress.value, ...event };
                },
            });
        } catch (e) {
            error.value = e;
        } finally {
            pending.value = false;
        }
    }

    async function analyzeUploadedFile() {
        if (!file.value) return;
        analyze(await file.value.text());
    }

    async function analyzeDemoScore() {
        file.value = null;
        const data = await $fetch(`/kern/bach-370-chorales/${DEMO_CHORALE_ID}.krn`, {
            parseResponse: (txt) => txt,
        });
        analyze(data);
    }

    return {
        file,
        options,
        pending,
        progress,
        error,
        result,
        position,
        segments,
        activeSegment,
        analyze,
        analyzeUploadedFile,
        analyzeDemoScore,
    };
}

const {
    file,
    options,
    pending,
    progress,
    error,
    result,
    position,
    segments,
    activeSegment,
    analyzeUploadedFile,
    analyzeDemoScore,
} = useSegmentAnalysis();

// What the extension says the file is. It gates submitting: a file no extension vouches for
// would only come back as the tool's parse error.
const fileFormat = computed(() => {
    const name = file.value?.name ?? '';
    const dot = name.lastIndexOf('.');
    const extension = dot === -1 ? '' : name.slice(dot).toLowerCase();
    if (KERN_EXTENSIONS.includes(extension)) return 'kern';
    if (MUSICXML_EXTENSIONS.includes(extension)) return 'musicxml';
    return null;
});

const fileSize = computed(() => (file.value ? formatBytes(file.value.size) : ''));

const fileError = computed(() => {
    if (!file.value) return null;
    if (file.value.size > MAX_FILE_SIZE) return t('fileTooLarge', { size: formatBytes(MAX_FILE_SIZE) });
    if (!fileFormat.value) return t('fileUnsupported');
    return null;
});

const activeStats = computed(() => activeSegment.value?.stats ?? null);

// The segments whose query found nothing anywhere in the corpus -- the passages this page
// exists to point at.
const unmatchedSegments = computed(() => segments.value.filter((segment) => segment.stats?.matches === 0));

// The unmatched segments as line ranges for the score, overlapping windows merged into one
// stretch each: 0-4, 1-5 and 2-6 all missing is one problem passage, not three markers deep.
const unmatchedRanges = computed(() => {
    const ranges = [];
    for (const segment of unmatchedSegments.value) {
        const last = ranges[ranges.length - 1];
        if (last && segment.startLine <= last.endLine) {
            last.endLine = Math.max(last.endLine, segment.endLine);
        } else {
            ranges.push({ startLine: segment.startLine, endLine: segment.endLine });
        }
    }
    return ranges;
});

// The score shows two things at once: every unmatched passage in red, and the segment being
// looked at in the default highlight. The windows overlap by design (0-4, 1-5, 2-6, ...), so
// beyond that the score stays unmarked -- every segment at once would bury it.
const activeSections = computed(() => {
    const groups = [];
    if (unmatchedRanges.value.length) {
        groups.push({
            items: unmatchedRanges.value.map((range) => ({ ...range })),
            color: highlightColorsByName.red,
        });
    }
    if (activeSegment.value) {
        groups.push({
            items: [
                {
                    startLine: activeSegment.value.startLine,
                    endLine: activeSegment.value.endLine,
                    label: activeSegment.value.id,
                },
            ],
            color: defaultHighlightColors[0],
        });
    }
    return groups;
});

// The passages behind a segment's counts, fetched only when asked for and kept per segment,
// so paging back to a segment doesn't search the corpus again.
const matchesBySegmentId = reactive({});
const matchesPending = ref(false);
const matchesVisible = ref(false);

watch(activeSegment, () => {
    matchesVisible.value = false;
});

// A new run reuses the segment ids ("segment-1", ...) for entirely different passages -- a
// changed length, another score -- so the cache from the previous run must not answer for them.
watch(result, () => {
    for (const key of Object.keys(matchesBySegmentId)) delete matchesBySegmentId[key];
    matchesVisible.value = false;
});

// The fetched matches of the segment on show, as [choraleId, matches] pairs, and how many
// matches they add up to.
const activeMatches = computed(() => matchesBySegmentId[activeSegment.value?.id] ?? []);
const activeMatchTotal = computed(() => activeMatches.value.reduce((sum, [, items]) => sum + items.length, 0));

async function showMatches() {
    const segment = activeSegment.value;
    if (!segment) return;
    if (!matchesBySegmentId[segment.id]) {
        matchesPending.value = true;
        try {
            const response = await $fetch('/api/chorale-search', {
                method: 'POST',
                body: segment.query,
            });
            matchesBySegmentId[segment.id] = Object.entries(response.results);
        } catch (e) {
            matchesBySegmentId[segment.id] = [];
        } finally {
            matchesPending.value = false;
        }
    }
    matchesVisible.value = true;
}

function sectionsForMatchItems(items) {
    return [
        {
            items: items.map((item) => ({
                voice: item.voice,
                startLine: item.startLine,
                endLine: item.endLine,
            })),
        },
    ];
}

// The segment's query or stats as JSON in a modal, so they don't take up room on the page.
const jsonOpen = ref(false);
const jsonTitle = ref('');
const jsonSegmentId = ref('');
const jsonData = ref(null);

function showJson(title, data) {
    jsonTitle.value = title;
    jsonSegmentId.value = activeSegment.value.id;
    jsonData.value = data;
    jsonOpen.value = true;
}

const { copy: copyToClipboard, copied: jsonCopied } = useClipboard({ copiedDuring: 2000 });

function goToSegment(n) {
    position.value = Math.min(Math.max(n, 1), segments.value.length);
}

// A clicked note selects the segment starting on its line. A note that opens no segment (an
// unclassified one, say) takes the latest segment that started before it.
function onNoteClick({ line }) {
    const exact = segments.value.findIndex((segment) => segment.startLine === line);
    const index = exact !== -1 ? exact : segments.value.findLastIndex((segment) => segment.startLine < line);
    if (index !== -1) goToSegment(index + 1);
}

// Arrow keys page through the segments, unless a modal is open or the cursor is in a field.
defineShortcuts({
    arrowleft: () => {
        if (!matchesVisible.value && !jsonOpen.value) goToSegment(position.value - 1);
    },
    arrowright: () => {
        if (!matchesVisible.value && !jsonOpen.value) goToSegment(position.value + 1);
    },
});

function onSubmit() {
    analyzeUploadedFile();
}
</script>

<template>
    <UContainer>
        <Heading>{{ $t('segmentAnalysis') }}</Heading>

        <UCard class="mb-4">
            <UForm class="space-y-4" @submit="onSubmit">
                <div class="max-w-md">
                    <p class="text-sm text-dimmed mb-2">{{ $t('uploadScoreDescription') }}</p>
                    <UFileUpload
                        v-model="file"
                        :accept="UPLOAD_ACCEPT"
                        :icon="file ? 'lucide:file-music' : undefined"
                        :label="file ? file.name : $t('uploadScoreLabel')"
                        :description="file ? fileSize : $t('uploadScoreFormats')"
                        :preview="false"
                        size="sm"
                        :ui="{ base: file ? 'min-h-14' : 'min-h-24', wrapper: file ? 'flex-row flex-wrap gap-x-3 gap-y-1' : '' }"
                        class="w-full"
                    >
                        <template v-if="file" #actions="{ removeFile }">
                            <UButton :label="$t('removeFile')" color="neutral" variant="soft" size="xs" @click.stop="removeFile()" />
                        </template>
                    </UFileUpload>
                    <UAlert v-if="fileError" color="warning" variant="subtle" icon="lucide:triangle-alert" :title="fileError" class="mt-2" />
                </div>

                <div class="grid gap-4 sm:grid-cols-2 lg:grid-cols-4">
                    <UFormField v-for="option in CHECK_OPTIONS" :key="option.key" :label="$t(option.label)" :description="$t(option.description)">
                        <USelect v-if="option.type === 'select'" v-model="options[option.key]" :items="option.items" class="w-full" />
                        <USwitch v-else-if="option.type === 'switch'" v-model="options[option.key]" />
                    </UFormField>
                </div>

                <div class="flex gap-2">
                    <UButton type="submit" :loading="pending" :disabled="!file || !!fileError">{{ $t('submit') }}</UButton>
                    <UButton v-if="isDev" color="neutral" variant="subtle" icon="lucide:flask-conical" :disabled="pending" @click="analyzeDemoScore">
                        {{ $t('useDemoScore', { id: DEMO_CHORALE_ID }) }}
                    </UButton>
                </div>
            </UForm>
        </UCard>

        <template v-if="error">
            <UAlert color="error" variant="subtle" :title="error.data?.message ?? $t('segmentAnalysisError')">
                <template v-if="error.data?.errors?.length" #description>
                    <ul>
                        <li v-for="(msg, i) in error.data.errors" :key="i">{{ msg }}</li>
                    </ul>
                </template>
            </UAlert>
        </template>
        <template v-else>
            <SearchProgress v-if="pending" :progress="progress" class="mt-8" />
            <UEmpty
                v-else-if="!result || segments.length === 0"
                :title="result ? $t('noSegments') : undefined"
                :description="$t('noSegmentsDescription')"
                icon="lucide:file-check"
                class="md:w-1/2 lg:w-1/3 mx-auto"
                :actions="[
                    {
                        icon: 'lucide:file-text',
                        label: $t('readDocs'),
                        to: localePath('/docs'),
                    },
                ]"
            />
            <template v-else>
                <div class="flex items-center justify-between gap-4 my-4">
                    <div class="flex items-center gap-2 text-sm">
                        <i18n-t keypath="segmentsFound" :plural="segments.length" tag="span" scope="global">
                            <template #segments>{{ segments.length }}</template>
                            <template #duration>
                                <span class="text-dimmed tabular-nums">({{ $t('searchDuration', { duration: formatDuration(result.durationMs) }) }})</span>
                            </template>
                        </i18n-t>
                        <UBadge v-if="unmatchedSegments.length" color="error" variant="subtle">
                            {{ $t('segmentsWithoutMatches', unmatchedSegments.length) }}
                        </UBadge>
                        <UBadge v-if="result.inputFormat === 'musicxml'" color="neutral" variant="subtle">{{ $t('convertedFromMusicxml') }}</UBadge>
                        <UBadge v-if="result.layout === 'grand-staff'" color="neutral" variant="subtle">{{ $t('splitIntoVoices') }}</UBadge>
                    </div>
                    <UPagination v-model:page="position" :total="segments.length" :items-per-page="1" size="xs" />
                </div>

                <div class="flex flex-col gap-4">
                    <UCard>
                        <HighlightedScore
                            :score-data="result.kern"
                            :horizontal="true"
                            :verovio-options="{
                                scale: 35,
                                pageMarginLeft: 42,
                                pageMarginTop: 130,
                            }"
                            :sections="activeSections"
                            :scroll-to-line="activeSegment?.startLine"
                            @note-click="onNoteClick"
                        />
                    </UCard>

                    <UCard v-if="activeSegment" class="w-full max-w-2xl mx-auto" :ui="{ body: 'p-3 sm:p-3' }">
                        <div class="grid gap-3 sm:grid-cols-[1fr_auto] sm:items-center">
                            <div class="flex flex-wrap items-center gap-x-3 gap-y-1">
                                <div class="flex items-center gap-1">
                                    <UButton icon="lucide:chevron-left" color="neutral" variant="subtle" size="xs" :aria-label="$t('previousSegment')" :disabled="position <= 1" @click="goToSegment(position - 1)" />
                                    <span class="text-sm tabular-nums px-1">{{ position }} / {{ segments.length }}</span>
                                    <UButton icon="lucide:chevron-right" color="neutral" variant="subtle" size="xs" :aria-label="$t('nextSegment')" :disabled="position >= segments.length" @click="goToSegment(position + 1)" />
                                </div>
                                <span class="font-semibold">{{ activeSegment.id }}</span>
                                <i18n-t
                                    v-if="activeStats"
                                    keypath="segmentStats"
                                    :plural="activeStats.choraleCount"
                                    tag="span"
                                    class="text-sm"
                                    :class="activeStats.matches === 0 ? 'text-error' : 'text-dimmed'"
                                    scope="global"
                                >
                                    <template #matches>{{ activeStats.matches }}</template>
                                </i18n-t>
                            </div>

                            <!-- Below the badges on small screens, next to the segment on larger ones. -->
                            <UFieldGroup class="order-last sm:order-none w-full sm:w-auto">
                                <UButton
                                    :label="$t('showMatches')"
                                    :loading="matchesPending"
                                    icon="lucide:search"
                                    color="neutral"
                                    variant="subtle"
                                    size="xs"
                                    class="flex-1 sm:flex-none justify-center"
                                    @click="showMatches"
                                />
                                <UButton icon="lucide:braces" color="neutral" variant="subtle" size="xs" :aria-label="$t('query')" :title="$t('query')" @click="showJson($t('query'), activeSegment.query)">
                                    <span class="hidden sm:inline">{{ $t('query') }}</span>
                                </UButton>
                                <UButton v-if="activeStats" icon="lucide:sigma" color="neutral" variant="subtle" size="xs" :aria-label="$t('stats')" :title="$t('stats')" @click="showJson($t('stats'), activeStats)">
                                    <span class="hidden sm:inline">{{ $t('stats') }}</span>
                                </UButton>
                            </UFieldGroup>

                            <div v-if="activeStats?.topChorales?.length" class="sm:col-span-2">
                                <p class="text-sm text-dimmed mb-2">{{ $t('topChorales') }}</p>
                                <div class="flex flex-wrap gap-2">
                                    <UFieldGroup v-for="entry in activeStats.topChorales" :key="entry.choraleId">
                                        <UBadge color="neutral" variant="subtle" :label="entry.choraleId" />
                                        <UBadge color="neutral" variant="outline" :label="$t('matchCount', entry.matches)" class="tabular-nums" />
                                    </UFieldGroup>
                                </div>
                            </div>
                        </div>
                    </UCard>
                </div>

            </template>
        </template>

        <UModal v-model:open="matchesVisible" :ui="{ content: 'sm:max-w-5xl' }">
            <template #title>
                <span class="inline-flex flex-wrap items-center gap-2">
                    <span>{{ $t('matchesInCorpus') }}</span>
                    <UBadge color="neutral" variant="subtle" :label="activeSegment?.id" />
                    <span class="text-sm font-normal text-dimmed">{{ $t('segmentStats', { matches: activeMatchTotal }, activeMatches.length) }}</span>
                </span>
            </template>
            <template #body>
                <div v-if="activeSegment" class="flex flex-col gap-4">
                    <UCard v-for="[choraleId, items] in activeMatches" :key="`${activeSegment.id}-${choraleId}`">
                        <template #header>
                            <div class="flex items-center justify-between gap-4">
                                <span>{{ choraleId }}</span>
                                <UBadge color="neutral" variant="subtle" :label="$t('matchCount', items.length)" />
                            </div>
                        </template>
                        <HighlightedScore
                            :horizontal="true"
                            :piece-id="choraleId"
                            :verovio-options="{
                                scale: 35,
                                pageMarginLeft: 42,
                            }"
                            :sections="sectionsForMatchItems(items)"
                            :scroll-to-first-section="true"
                        />
                    </UCard>
                </div>
            </template>
        </UModal>

        <UModal v-model:open="jsonOpen" :ui="{ content: 'sm:max-w-3xl' }">
            <template #title>
                <span class="inline-flex flex-wrap items-center gap-2">
                    <span>{{ jsonTitle }}</span>
                    <UBadge color="neutral" variant="subtle" :label="jsonSegmentId" />
                </span>
            </template>
            <template #body>
                <pre class="text-xs overflow-auto max-h-[60vh] rounded-md bg-elevated p-4">{{ JSON.stringify(jsonData, null, 4) }}</pre>
            </template>
            <template #footer>
                <UButton
                    :icon="jsonCopied ? 'lucide:check' : 'lucide:copy'"
                    :label="jsonCopied ? $t('copied') : $t('copy')"
                    color="neutral"
                    variant="subtle"
                    @click="copyToClipboard(JSON.stringify(jsonData, null, 4))"
                />
            </template>
        </UModal>
    </UContainer>
</template>
