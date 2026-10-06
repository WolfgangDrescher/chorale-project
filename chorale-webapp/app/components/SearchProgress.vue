<script setup lang="ts">
// Progress of a chorale-search or chorale-segment run, from the events the tool reports.
interface Progress {
    phase?: string; // the stage being worked on (chorale-segment only)
    chorale?: string; // the chorale file searched last
    choralesSearched?: number;
    choralesTotal?: number;
    matchesSoFar?: number;
}

const props = defineProps<{ progress: Progress | null }>();

const { t } = useI18n();

const PHASE_TITLES: Record<string, string> = {
    'convert-musicxml': 'progressConvertMusicxml',
    'split-score-into-voices': 'progressSplitScoreIntoVoices',
    'analyze-score': 'progressAnalyzeScore',
    'segment-score': 'progressSegmentScore',
    'run-checks': 'progressRunChecks',
    'search-corpus': 'progressSearchCorpus',
    'collect-bass-lines': 'progressCollectBassLines',
};

// chorale-search reports no phases, so its first search progress is what tells the search began.
const corpusSearchStarted = computed(() => props.progress?.choralesSearched !== undefined);

// The phase on show wins, which also lets a second corpus pass (the bassLines) name
// itself; without one, a started search is the plain corpus search.
const title = computed(() => {
    const key = PHASE_TITLES[props.progress?.phase ?? ''];
    if (key) return t(key);
    return corpusSearchStarted.value ? t(PHASE_TITLES['search-corpus']) : '';
});

const choralesText = computed(() => t('progressChoralesSearched', {
    searched: props.progress?.choralesSearched,
    total: props.progress?.choralesTotal,
}));

const matchesText = computed(() => t('progressMatchesSoFar', props.progress?.matchesSoFar ?? 0));
</script>

<template>
    <div class="flex flex-col gap-2 max-w-lg mx-auto">
        <UProgress
            :model-value="progress?.choralesTotal ? progress.choralesSearched : null"
            :max="progress?.choralesTotal ?? 100"
            size="2xl"
        />
        <div v-if="title" class="flex items-baseline justify-between gap-4">
            <p class="text-sm font-medium">{{ title }}</p>
            <span v-if="corpusSearchStarted" class="text-xs text-dimmed font-mono">{{ progress.chorale }}</span>
        </div>
        <div v-if="corpusSearchStarted" class="flex justify-between gap-4 text-sm text-dimmed tabular-nums">
            <span>{{ choralesText }}</span>
            <span>{{ matchesText }}</span>
        </div>
        <div v-if="$slots.default" class="flex justify-center">
            <slot />
        </div>
    </div>
</template>
