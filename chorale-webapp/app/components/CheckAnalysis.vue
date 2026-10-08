<script setup>
// The findings of the checks on a score. The server prepares the score (a MusicXML file converted,
// a two-staff one split into four voices) and runs the checks of chorale-check on it (see
// Check.hpp). Here they are only shown.

// The score, shared with the other analyses of the page.
const file = defineModel('file', { default: null });

// Whether this analysis is the one on show: the tabs stay mounted, so the keys must be left to the
// other one while it is not.
const props = defineProps({
    active: { type: Boolean, default: true },
});

const { fileError } = useScoreFile(file);

// The checks that find something, as the keys this page translates. Each finding names the one it
// comes from.
const CHECK_TYPES = ['parallelFifths', 'parallelOctaves', 'hiddenFifths', 'hiddenOctaves', 'voiceCrossing', 'largeLeap', 'bassPhraseEnd', 'voiceRange'];

// What the checker looks for, listed next to the upload. A new check is an entry here and its two
// translations.
const CHECKS = [
    { key: 'checkParallelMotion', label: 'checkParallelMotion', description: 'checkParallelMotionDescription' },
    { key: 'checkHiddenMotion', label: 'checkHiddenMotion', description: 'checkHiddenMotionDescription' },
    { key: 'checkVoiceCrossing', label: 'checkVoiceCrossing', description: 'checkVoiceCrossingDescription' },
    { key: 'checkLargeLeaps', label: 'checkLargeLeaps', description: 'checkLargeLeapsDescription' },
    { key: 'checkBassPhraseEnd', label: 'checkBassPhraseEnd', description: 'checkBassPhraseEndDescription' },
    { key: 'checkVoiceRange', label: 'checkVoiceRange', description: 'checkVoiceRangeDescription' },
];

const SEVERITY_BADGE_COLORS = { error: 'error', warning: 'warning' };

const DIRECTION_LABELS = { up: 'directionUp', down: 'directionDown', above: 'directionAbove', below: 'directionBelow' };

// The voice ranges the notes can be measured against, the first being the one to begin with. Each is
// a tab of the choice under the upload, and its range is explained in the popover next to it.
const VOICE_RANGE_SETS = [
    { key: 'strauss-berlioz', label: 'voiceRangesStraussBerlioz', description: 'voiceRangesStraussBerliozDescription' },
    { key: 'bach', label: 'voiceRangesBach', description: 'voiceRangesBachDescription' },
];

const voiceRanges = ref(VOICE_RANGE_SETS[0].key);

// Whether a hidden fifth or octave is excused when the soprano reaches it by a step.
const hiddenMotionAllowSteps = ref(true);

const { t } = useI18n();
const voiceRangeTabs = VOICE_RANGE_SETS.map((set) => ({ value: set.key, label: t(set.label) }));

const VOICE_NAMES = { 1: 'voiceBass', 2: 'voiceTenor', 3: 'voiceAlto', 4: 'voiceSoprano' };

// The lines of the findings that run from one note to another in the score, and the markers of the
// notes out of their range.
const ERROR_COLOR = 'rgb(239 68 68)';
const LINE_WIDTH = 3;
const WARNING_COLOR = highlightColorsByName.amber;

// The frame of the error on show, in the primary color like the segment on show of the segment
// analysis.
const ACTIVE_FRAME_COLOR = 'color-mix(in oklab, var(--ui-primary) 80%, transparent)';

const pending = ref(false);
const progress = ref(null);
const error = ref(null);
const result = ref(null); // { kern, findings, durationMs }

// 1-based, so it doubles as UPagination's page with one error per page.
const position = ref(1);

const findings = computed(() => result.value?.findings ?? []);
const activeFinding = computed(() => findings.value[position.value - 1] ?? null);

const countsByType = computed(() =>
    CHECK_TYPES.map((type) => {
        const ofType = findings.value.filter((entry) => entry.check === type);
        return { type, count: ofType.length, color: SEVERITY_BADGE_COLORS[ofType[0]?.severity] };
    }).filter((entry) => entry.count > 0),
);

let abortController = null;

function cancel() {
    abortController?.abort();
}

async function analyze() {
    if (!file.value) return;
    abortController = new AbortController();
    pending.value = true;
    progress.value = null;
    error.value = null;
    result.value = null;
    position.value = 1;
    try {
        const data = await file.value.text();
        const response = await fetchWithProgress('/api/chorale-check', {
            body: { data, voiceRanges: voiceRanges.value, hiddenMotionAllowSteps: hiddenMotionAllowSteps.value },
            signal: abortController.signal,
            onProgress: (event) => {
                progress.value = { ...progress.value, ...event };
            },
        });
        result.value = {
            kern: response.kern,
            findings: response.findings,
            durationMs: response.durationMs,
        };
    } catch (e) {
        // A cancelled run is what the person asked for, not a failure to report.
        if (e.name !== 'AbortError') error.value = e;
    } finally {
        abortController = null;
        pending.value = false;
    }
}

// The checks shown as a section over the notes of the voice they are about: the bass note under the
// fermata and the note before it.
const SECTION_CHECKS = ['bassPhraseEnd'];

// What a finding marks in the score: one voice for a note, two for a parallel or a crossing.
const voicesOf = (entry) => [...new Set(entry.voices)];

// The two lines of a parallel, one for each voice, running horizontally from its first note to its
// second one.
function linesOf(entry) {
    return voicesOf(entry).map((voice) => ({
        from: { line: entry.startLine, voice },
        to: { line: entry.endLine, voice },
    }));
}

// Findings that run from one note to another are drawn as lines, those about a single note as a
// marker on it.
const spansNotes = (entry) => entry.startLine !== entry.endLine;

const connections = computed(() => [
    { severity: 'error', color: ERROR_COLOR },
    { severity: 'warning', color: WARNING_COLOR },
].map(({ severity, color }) => ({
    items: findings.value
            .filter((entry) => spansNotes(entry) && !SECTION_CHECKS.includes(entry.check) && entry.severity === severity)
            .flatMap(linesOf),
    color,
    width: LINE_WIDTH,
})));

const notes = computed(() => [
    {
        items: findings.value
            .filter((entry) => !spansNotes(entry))
            .flatMap((entry) => voicesOf(entry).map((voice) => `L${entry.startLine}F${voice}`)),
        color: WARNING_COLOR,
    },
]);

// A section over the bass at every missing cadence bass, and a frame around the notes of each voice of
// the finding on show.
const sections = computed(() => {
    const groups = [
        {
            items: findings.value
                .filter((entry) => SECTION_CHECKS.includes(entry.check))
                .map((entry) => ({ voice: entry.voices[0], startLine: entry.startLine, endLine: entry.endLine })),
            color: WARNING_COLOR,
        },
    ];
    if (activeFinding.value) {
        const { startLine, endLine } = activeFinding.value;
        groups.push({ items: voicesOf(activeFinding.value).map((voice) => ({ voice, startLine, endLine })), color: ACTIVE_FRAME_COLOR, outline: true, fitBoundingBox: true });
    }
    return groups;
});

// The line to keep in view for the finding on show.
const scrollToLine = computed(() => (activeFinding.value?.startLine));

function goToFinding(n) {
    position.value = Math.min(Math.max(n, 1), findings.value.length);
}

// The left and right arrow keys page through the findings.
defineShortcuts({
    arrowleft: () => props.active && goToFinding(position.value - 1),
    arrowright: () => props.active && goToFinding(position.value + 1),
});

// A clicked note selects the first finding that has it among its notes.
function onNoteClick({ line, voice }) {
    const index = findings.value.findIndex(
        (entry) => entry.voices.includes(voice) && (entry.startLine === line || entry.endLine === line),
    );
    if (index !== -1) goToFinding(index + 1);
}
</script>

<template>
    <div>
        <UCard class="mb-4">
            <UForm class="space-y-4" @submit="analyze">
                <div class="grid gap-6 lg:grid-cols-[1fr_auto_2fr] lg:items-start">
                    <ScoreFileField v-model="file" />

                    <USeparator class="lg:hidden" />
                    <USeparator orientation="vertical" class="hidden lg:flex lg:self-stretch" />

                    <div class="min-w-0">
                        <p class="text-sm mb-2">{{ $t('checkListTitle') }}</p>
                        <div class="px-12">
                            <UCarousel
                                v-slot="{ item }"
                                :items="CHECKS"
                                arrows
                                align="start"
                                :ui="{ item: 'basis-full sm:basis-1/2', prev: '-start-12', next: '-end-12' }"
                                @keydown.stop
                            >
                                <p class="font-semibold text-sm">{{ $t(item.label) }}</p>
                                <p class="text-sm">{{ $t(item.description) }}</p>
                                <USwitch
                                    v-if="item.key === 'checkHiddenMotion'"
                                    v-model="hiddenMotionAllowSteps"
                                    :label="$t('hiddenMotionAllowSteps')"
                                    size="sm"
                                    class="mt-3"
                                />
                                <div v-else-if="item.key === 'checkVoiceRange'" class="mt-3">
                                    <UTabs v-model="voiceRanges" :items="voiceRangeTabs" :content="false" size="xs" />
                                    <p class="text-sm mt-2">{{ $t(VOICE_RANGE_SETS.find((set) => set.key === voiceRanges).description) }}</p>
                                </div>
                            </UCarousel>
                        </div>
                    </div>
                </div>

                <UButton type="submit" :loading="pending" :disabled="!file || !!fileError">{{ $t('submit') }}</UButton>
            </UForm>
        </UCard>

        <UAlert v-if="error" color="error" variant="subtle" :title="error.data?.message ?? $t('checkError')">
            <template v-if="error.data?.errors?.length" #description>
                <ul>
                    <li v-for="(msg, i) in error.data.errors" :key="i">{{ msg }}</li>
                </ul>
            </template>
        </UAlert>
        <SearchProgress v-else-if="pending" :progress="progress" class="mt-8">
            <UButton color="neutral" variant="soft" size="xs" icon="lucide:x" @click="cancel">{{ $t('cancel') }}</UButton>
        </SearchProgress>
        <UEmpty
            v-else-if="!result"
            :description="$t('checkEmptyDescription')"
            icon="lucide:file-check"
            class="md:w-1/2 lg:w-1/3 mx-auto"
        />
        <template v-else>
            <div class="flex items-center justify-between gap-4 my-4">
                <div class="flex flex-wrap items-center gap-2 text-sm">
                    <span v-if="!findings.length">{{ $t('noCheckFindings') }}</span>
                    <template v-else>
                        <span>{{ $t('checkFindingsFound', findings.length) }}</span>
                        <UBadge v-for="entry in countsByType" :key="entry.type" :color="entry.color" variant="subtle">
                            {{ $t(`${entry.type}Count`, entry.count) }}
                        </UBadge>
                    </template>
                </div>
                <UPagination v-if="findings.length" v-model:page="position" :total="findings.length" :items-per-page="1" size="xs" />
            </div>

            <div class="flex flex-col gap-4">
                <UCard>
                    <HighlightedScore
                        :score-data="result.kern"
                        :horizontal="true"
                        :verovio-options="{
                            scale: 35,
                            pageMarginLeft: 42,
                            pageMarginTop: 60,
                        }"
                        :connections="connections"
                        :notes="notes"
                        :sections="sections"
                        :scroll-to-line="scrollToLine"
                        @note-click="onNoteClick"
                    />
                </UCard>

                <UCard v-if="findings.length" class="w-full max-w-2xl mx-auto" :ui="{ body: 'p-3 sm:p-3' }">
                    <ul class="divide-y divide-default">
                        <li v-for="(entry, index) in findings" :key="index">
                            <button
                                type="button"
                                class="flex w-full items-center gap-3 px-2 py-1.5 text-left text-sm rounded hover:bg-elevated"
                                :class="index === position - 1 && 'bg-elevated font-semibold'"
                                @click="goToFinding(index + 1)"
                            >
                                <span class="tabular-nums text-dimmed w-8">{{ index + 1 }}</span>
                                <UBadge :color="SEVERITY_BADGE_COLORS[entry.severity]" variant="subtle">{{ $t(entry.check) }}</UBadge>
                                <span v-if="entry.direction">{{ $t(DIRECTION_LABELS[entry.direction]) }}</span>
                                <span class="text-muted">{{ voicesOf(entry).map((voice) => $t(VOICE_NAMES[voice])).join(' / ') }}</span>
                                <span class="ml-auto text-dimmed tabular-nums">{{ $t('lineNumber', { line: entry.startLine }) }}</span>
                            </button>
                        </li>
                    </ul>
                </UCard>
            </div>
        </template>
    </div>
</template>
