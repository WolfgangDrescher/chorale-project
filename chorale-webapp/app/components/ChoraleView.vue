<script setup>
// The score of a Bach chorale with its palette, player, downloads and modulations, on its page and in a drawer.
const props = defineProps({
    // An item of the bachChorales collection.
    chorale: { type: Object, required: true },
    // The sections to highlight, see the matches of the chorale page.
    matches: { type: Array, default: () => [] },
    // The pages of the neighbouring chorales for the arrow keys, or null.
    prevRoute: { type: [String, Object], default: null },
    nextRoute: { type: [String, Object], default: null },
});

const annotating = defineModel('annotating', { type: Boolean, default: false });

const { t } = useI18n();
const chorale = toRef(props, 'chorale');
const scoreOptions = useScoreOptions();
const keyNotation = useKeyNotation();
const { localScoreUrlGenerator, githubScoreUrlGenerator, vhvScoreUrlGenerator } = useScoreUrlGenerator();

const {
    exportFormat,
    exportContent,
    exportVoices,
    exportLayout,
    pageOrientation,
    exportFormatItems,
    exportContentItems,
    exportVoicesItems,
    exportLayoutItems,
    pageOrientationItems,
    downloadItems,
} = useChoraleExport(chorale);

const player = useScorePlayer();
useScorePlayerKeyboardShortcuts(() => player);

const modulations = computed(() => chorale.value.modulations ?? []);

const modulationLines = computed(() => [{
    color: 'rgb(34 197 94 / 0.4)',
    items: modulations.value.map(({ line, key, mode }) => ({
        lineNumber: line,
        label: { value: key === '?' ? key : keyNameInNotation(key, mode, keyNotation.value, t), position: 'bottom' },
    })),
}]);

// The modulations are annotated here, see useModulationAnnotator; the development server writes them to the file.
const annotator = useModulationAnnotator(chorale.value.choraleId, player, annotating, import.meta.dev);

useChoraleKeyboardShortcuts({ prevRoute: () => props.prevRoute, nextRoute: () => props.nextRoute, keysTaken: annotating });

// The diminutions as the queries find them or as annotated by hand, and annotated with clicks on the notes.
const diminutionSource = ref('found');
const annotatingDiminutions = ref(false);
const {
    notes: diminutionNotes,
    sections: diminutionSections,
    legend: diminutionLegend,
    annotator: diminutionAnnotator,
    annotateFound: annotateFoundDiminutions,
} = useDiminutionAnalysis(chorale.value.choraleId, () => (scoreOptions.showDiminutions ? scoreOptions.diminutions : []), { source: diminutionSource, annotating: annotatingDiminutions });
const hasAnnotatedDiminutions = computed(() => chorale.value.choraleId in DIMINUTION_ANNOTATIONS);

// A click to annotate goes to its note without showing it there, which would follow every click, until the music plays.
const playedWhileAnnotating = ref(false);
watch(annotatingDiminutions, () => {
    playedWhileAnnotating.value = false;
});
watch(() => player.isPlaying, (playing) => {
    if (playing) playedWhileAnnotating.value = true;
});

function annotateDiminution({ line, voice, held }) {
    if (!scoreOptions.showDiminutions || !scoreOptions.diminutions.includes(diminutionAnnotator.current)) return;
    diminutionAnnotator.click({ line: line - lineShiftOf(scoreOptions.humdrumFilters), voice, held });
}

const verovioOptions = computed(() => ({ ...scoreOptions.verovioOptions, ...CHORALE_PAGE_VEROVIO_OPTIONS }));
</script>

<template>
    <div class="flex flex-col gap-8">
        <div class="flex flex-wrap items-center gap-x-6 gap-y-3">
            <ScoreOptionsPalette :chorale="chorale" />
            <ScorePlayerControls :player="player" :file-name="chorale.choraleId" />
            <div class="shrink-0 flex gap-2 ml-auto">
                <ChoraleScoreLinks :github-url="githubScoreUrlGenerator(chorale.choraleId)" :vhv-url="vhvScoreUrlGenerator(chorale.choraleId)">
                    <UTooltip v-if="chorale.urlScan" :text="$t('scan')">
                        <UButton :to="chorale.urlScan" target="_blank" icon="lucide:book-image" color="neutral" variant="outline" :aria-label="$t('scan')" />
                    </UTooltip>
                </ChoraleScoreLinks>
                <UDropdownMenu :items="downloadItems" arrow>
                    <UButton icon="lucide:download" trailing-icon="lucide:chevron-down" color="neutral" variant="outline" :aria-label="$t('download')" />
                    <template #content-top>
                        <div class="flex flex-col gap-2 p-1.5 pb-1">
                            <div>
                                <div class="text-xs text-muted mb-1">
                                    {{ $t('exportFormat') }}
                                </div>
                                <UTabs v-model="exportFormat" :items="exportFormatItems" :content="false" size="xs" class="w-full" />
                            </div>
                            <div>
                                <div class="text-xs text-muted mb-1">
                                    {{ $t('exportContent') }}
                                </div>
                                <UTabs v-model="exportContent" :items="exportContentItems" :content="false" size="xs" class="w-full" />
                            </div>
                            <div v-if="exportFormat === 'mei'">
                                <div class="text-xs text-muted mb-1">
                                    {{ $t('exportVoices') }}
                                </div>
                                <UTabs v-model="exportVoices" :items="exportVoicesItems" :content="false" size="xs" class="w-full" />
                            </div>
                            <div>
                                <div class="text-xs text-muted mb-1">
                                    {{ $t('exportLayout') }}
                                </div>
                                <UTabs v-model="exportLayout" :items="exportLayoutItems" :content="false" size="xs" class="w-full" />
                            </div>
                            <div v-if="exportFormat === 'pdf'">
                                <div class="text-xs text-muted mb-1">
                                    {{ $t('pageOrientation') }}
                                </div>
                                <UTabs v-model="pageOrientation" :items="pageOrientationItems" :content="false" size="xs" class="w-full" />
                            </div>
                        </div>
                    </template>
                </UDropdownMenu>
            </div>
        </div>

        <DiminutionLegend
            v-if="scoreOptions.showDiminutions || annotatingDiminutions"
            v-model:source="diminutionSource"
            v-model:annotating="annotatingDiminutions"
            :legend="diminutionLegend"
            :annotator="diminutionAnnotator"
            :annotated="hasAnnotatedDiminutions"
            @toggle="scoreOptions.toggleDiminution"
            @annotate-found="annotateFoundDiminutions"
        />

        <PlayableHighlightedScore
            :player="player"
            :score-url="localScoreUrlGenerator(chorale.choraleId)"
            :horizontal="scoreOptions.showHorizontalViewMode"
            :verovio-options="verovioOptions"
            :base-filters="scoreOptions.baseFilters"
            :filters="scoreOptions.humdrumFilters"
            :sections="[{ items: matches }, ...diminutionSections]"
            :notes="diminutionNotes"
            :lines="annotator.active ? annotator.markers : (scoreOptions.showModulations ? modulationLines : undefined)"
            :scroll-to-first-section="matches.length > 0"
            :shows-position="!annotatingDiminutions || playedWhileAnnotating"
            :clickable-held-notes="annotatingDiminutions"
            @note-click="annotateDiminution"
        />

        <ModulationAnnotator v-if="annotator.active" :annotator="annotator" />

        <ModulationTimeline
            v-model:annotating="annotating"
            :player="player"
            :modulations="annotator.modulations ?? modulations"
            :cantus-firmus="chorale.cantusFirmus"
            :line-shift="lineShiftOf(scoreOptions.humdrumFilters)"
            :outdated="annotator.active && !annotator.saves && annotator.changed"
        />
    </div>
</template>
