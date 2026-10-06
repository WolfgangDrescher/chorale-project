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
// key in server/api/chorale-segment.js. An option with a place in the docs gets a badge linking
// there (see SegmentOptionField). A `disabled` option is one the segment queries always use
// (see defaultSegmentMatcherOptions): it is listed as a disabled switch so the help can explain
// it, and it isn't sent along.
const CHECK_OPTIONS = [
    {
        key: 'length',
        type: 'select',
        items: SEGMENT_LENGTHS,
        default: 4,
        label: 'segmentLength',
        description: 'segmentLengthDescription',
    },
    {
        key: 'ignoreIntervalQuality',
        type: 'switch',
        default: true,
        label: 'ignoreIntervalQuality',
        docs: { label: '+M2 → +2', path: '/docs/features/mint', hash: 'quality--and-sign-optional-matching' },
        description: 'ignoreIntervalQualityDescription',
    },
    {
        key: 'allowIntervalComplementation',
        type: 'switch',
        default: true,
        label: 'allowIntervalComplementation',
        query: 'mintAllowIntervalComplementation',
        description: 'allowIntervalComplementationDescription',
    },
    {
        key: 'skipUnclassifiedBeats',
        type: 'switch',
        default: true,
        label: 'skipUnclassifiedBeats',
        query: 'metweightSkipUnclassified',
        description: 'skipUnclassifiedBeatsDescription',
    },
    {
        key: 'hintReduceCompound',
        type: 'switch',
        disabled: true,
        label: 'hintReduceCompound',
        query: 'hintReduceCompound',
        description: 'hintReduceCompoundDescription',
    },
    {
        key: 'durationAllowSplitNotes',
        type: 'switch',
        disabled: true,
        label: 'durationAllowSplitNotes',
        query: 'durationAllowSplitNotes',
        description: 'durationAllowSplitNotesDescription',
    },
    {
        key: 'durationAllowMergedNotes',
        type: 'switch',
        disabled: true,
        label: 'durationAllowMergedNotes',
        query: 'durationAllowMergedNotes',
        description: 'durationAllowMergedNotesDescription',
    },
];

// The selects stay under the dropzone, the switches go beside it on large screens.
const SELECT_OPTIONS = CHECK_OPTIONS.filter((option) => option.type === 'select');
const SWITCH_OPTIONS = CHECK_OPTIONS.filter((option) => option.type === 'switch');

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
    const options = reactive(
        Object.fromEntries(CHECK_OPTIONS.filter((option) => !option.disabled).map((option) => [option.key, option.default])),
    );
    const pending = ref(false);
    const progress = ref(null);
    const error = ref(null);
    const result = ref(null); // { kern, segments, durationMs }

    // 1-based, so it doubles as UPagination's page with one segment per page.
    const position = ref(1);

    const segments = computed(() => result.value?.segments ?? []);
    const activeSegment = computed(() => segments.value[position.value - 1] ?? null);

    // Set while a run is on the way; aborting it stops the request and with it the tool.
    let abortController = null;

    function cancel() {
        abortController?.abort();
    }

    async function analyze(data) {
        abortController = new AbortController();
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
                signal: abortController.signal,
                onProgress: (event) => {
                    progress.value = { ...progress.value, ...event };
                },
            });
        } catch (e) {
            // A cancelled run is what the person asked for, not a failure to report.
            if (e.name !== 'AbortError') error.value = e;
        } finally {
            abortController = null;
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
        cancel,
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
    cancel,
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

// A segment found this often in the corpus or less (but at least once) counts as rare.
const RARE_MATCH_THRESHOLD = 2;

// The frame of the segment on show lies over the tier markers, so it is the primary color with
// little transparency: at 40% it would wash out against them.
const ACTIVE_FRAME_COLOR = 'color-mix(in oklab, var(--ui-primary) 80%, transparent)';

// How a segment's match count marks it in the score, rarest first. Later tiers are drawn over
// earlier ones where they overlap.
const MATCH_TIERS = [
    {
        key: 'unmatched',
        color: highlightColorsByName.red,
        badgeColor: 'error',
        badge: 'segmentsWithoutMatches',
        legend: 'legendUnmatched',
        includes: (matches) => matches === 0,
    },
    {
        key: 'rare',
        color: highlightColorsByName.amber,
        badgeColor: 'warning',
        badge: 'segmentsWithFewMatches',
        legend: 'legendRare',
        includes: (matches) => matches > 0 && matches <= RARE_MATCH_THRESHOLD,
    },
];

function tierOf(segment) {
    const matches = segment.stats?.matches;
    if (matches === undefined) return null;
    return MATCH_TIERS.find((tier) => tier.includes(matches)) ?? null;
}

// The segments of each tier -- the passages this page exists to point at.
const segmentsByTier = computed(() =>
    Object.fromEntries(MATCH_TIERS.map((tier) => [tier.key, segments.value.filter((segment) => tierOf(segment) === tier)])),
);

// A tier's segments as line ranges for the score, overlapping windows merged into one stretch
// each: 0-4, 1-5 and 2-6 all missing is one problem passage, not three markers deep.
function mergeIntoRanges(tierSegments) {
    const ranges = [];
    for (const segment of tierSegments) {
        const last = ranges[ranges.length - 1];
        if (last && segment.startLine <= last.endLine) {
            last.endLine = Math.max(last.endLine, segment.endLine);
        } else {
            ranges.push({ startLine: segment.startLine, endLine: segment.endLine });
        }
    }
    return ranges;
}

// The score shows the marked passages of every tier at once (red: not in the corpus at all,
// amber: rare) and the segment being looked at as a frame in the primary color. The windows overlap by
// design (0-4, 1-5, 2-6, ...), so beyond that the score stays unmarked -- every segment at once
// would bury it.
const activeSections = computed(() => {
    const groups = [];
    for (const tier of [...MATCH_TIERS].reverse()) {
        const ranges = mergeIntoRanges(segmentsByTier.value[tier.key]);
        if (ranges.length) groups.push({ items: ranges, color: tier.color });
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
            color: ACTIVE_FRAME_COLOR,
            outline: true,
        });
    }
    return groups;
});

// What the legend under the score explains: the tiers plus the segment on show.
const legendItems = [
    ...MATCH_TIERS.map((tier) => ({ key: tier.key, color: tier.color, label: tier.legend })),
    { key: 'active', color: ACTIVE_FRAME_COLOR, label: 'legendActive', outline: true },
];

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

// Every chorale on show is a rendered score, so the modal lists them a page at a time.
const MATCHES_PER_PAGE = 10;
const matchesPage = ref(1); // 1-based, UPagination's page
const pagedMatches = computed(() => {
    const start = (matchesPage.value - 1) * MATCHES_PER_PAGE;
    return activeMatches.value.slice(start, start + MATCHES_PER_PAGE);
});

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
    matchesPage.value = 1;
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
                <div class="grid gap-6 lg:grid-cols-[1fr_auto_2fr] lg:items-start">
                    <div>
                        <p class="text-sm mb-2">{{ $t('uploadScoreDescription') }}</p>
                        <div class="relative">
                            <UFileUpload
                                v-model="file"
                                :accept="UPLOAD_ACCEPT"
                                :icon="file ? 'lucide:file-music' : undefined"
                                :label="file ? file.name : $t('uploadScoreLabel')"
                                :description="file ? fileSize : $t('uploadScoreFormats')"
                                :preview="false"
                                size="sm"
                                :ui="{ base: 'min-h-24' }"
                                class="w-full"
                            />
                            <UButton
                                v-if="file"
                                icon="lucide:x"
                                color="neutral"
                                variant="ghost"
                                size="xs"
                                class="absolute top-2 right-2"
                                :aria-label="$t('removeFile')"
                                :title="$t('removeFile')"
                                @click="file = null"
                            />
                        </div>
                        <UAlert v-if="fileError" color="warning" variant="subtle" icon="lucide:triangle-alert" :title="fileError" class="mt-2" />

                        <SegmentOptionField
                            v-for="option in SELECT_OPTIONS"
                            :key="option.key"
                            v-model="options[option.key]"
                            :option="option"
                            class="mt-4"
                        />
                    </div>

                    <USeparator class="lg:hidden" />
                    <USeparator orientation="vertical" class="hidden lg:flex lg:self-stretch" />

                    <div class="grid grid-cols-2 gap-x-6 gap-y-4">
                        <SegmentOptionField
                            v-for="option in SWITCH_OPTIONS"
                            :key="option.key"
                            v-model="options[option.key]"
                            :option="option"
                        />
                    </div>
                </div>

                <div class="flex gap-2">
                    <UButton type="submit" :loading="pending" :disabled="!file || !!fileError">{{ $t('submit') }}</UButton>
                    <UButton v-if="isDev && !file" color="neutral" variant="subtle" icon="lucide:flask-conical" :disabled="pending" @click="analyzeDemoScore">
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
            <SearchProgress v-if="pending" :progress="progress" class="mt-8">
                <UButton color="neutral" variant="soft" size="xs" icon="lucide:x" @click="cancel">{{ $t('cancel') }}</UButton>
            </SearchProgress>
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
                        <UBadge v-for="tier in MATCH_TIERS" v-show="segmentsByTier[tier.key].length" :key="tier.key" :color="tier.badgeColor" variant="subtle">
                            {{ $t(tier.badge, segmentsByTier[tier.key].length) }}
                        </UBadge>
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
                        <ul class="flex flex-wrap gap-x-5 gap-y-1 mt-3 text-xs text-muted">
                            <li v-for="item in legendItems" :key="item.key" class="flex items-center gap-1.5">
                                <span
                                    class="inline-block size-3 rounded-sm"
                                    :class="item.outline && 'border-[3px] border-solid'"
                                    :style="item.outline ? { borderColor: item.color } : { backgroundColor: item.color }"
                                />
                                {{ $t(item.label, { count: RARE_MATCH_THRESHOLD }) }}
                            </li>
                        </ul>
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
                                <UBadge v-if="activeStats" :color="tierOf(activeSegment)?.badgeColor ?? 'neutral'" variant="subtle">
                                    <i18n-t
                                        keypath="segmentStats"
                                        :plural="activeStats.choraleCount"
                                        tag="span"
                                        scope="global"
                                    >
                                        <template #matches>{{ activeStats.matches }}</template>
                                    </i18n-t>
                                </UBadge>
                            </div>

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
                    <UCard v-for="[choraleId, items] in pagedMatches" :key="`${activeSegment.id}-${choraleId}`">
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
                    <UPagination
                        v-if="activeMatches.length > MATCHES_PER_PAGE"
                        v-model:page="matchesPage"
                        :total="activeMatches.length"
                        :items-per-page="MATCHES_PER_PAGE"
                        size="xs"
                        class="self-center"
                    />
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
