import { defineStore, acceptHMRUpdate } from 'pinia';

function createDefaultScoreOptions() {
    return {
        showMeter: false,
        satb2gs: false,
        intervallsatz: false,
        extendedFiguredbass: false,
        bassScaleDegree: false,
        scaleDegree: false,
        hideMiddleVoices: false,
        extractCantusFirmus: false,
        hideNonCantusFirmusVoices: false,
        // The key the score is transposed to, as **kern spells its tonic (e.g. "e-" or "f#"), or
        // null for the key it is written in. The mode stays as it is.
        transposeKey: null,
        verovioScale: 40,
        showHorizontalViewMode: false,
    };
}

// The score options of a store with its own defaults, so the search results do not share the
// options of the page of a chorale.
const scoreOptionsStore = (createDefaults) => ({
    state: () => createDefaults(),
    getters: {
        // The Humdrum filters the score can be shown through, by the name of the option that
        // toggles them. A filter with a higher priority is applied first, so the score is
        // transposed and its voices are extracted before the remaining spines are analyzed. The
        // command of an option with a value (not true or false) is made from the state.
        humdrumFilterDefinitions: () => ({
            transposeKey: { command: (state) => `transpose -k ${state.transposeKey}`, priority: 3 },
            showMeter: { command: 'meter -f' },
            satb2gs: { command: 'satb2gs', priority: 1 },
            intervallsatz: { command: 'fb -icatm', priority: -1 },
            extendedFiguredbass: { command: 'fb -acon3' },
            bassScaleDegree: { command: 'deg --circle -k 1' },
            scaleDegree: { command: 'deg --circle' },
            hideMiddleVoices: { command: 'extract -f 1,$', priority: 2 },
            extractCantusFirmus: { command: 'extract -f $', priority: 2 },
            hideNonCantusFirmusVoices: {
                command: 'shed -s 1-3 -e "s/.*/$0yy/D s/.*//L s/^[^I].*//I"',
                priority: 2,
            },
        }),
        humdrumFilterMap(state) {
            return Object.fromEntries(
                Object.entries(this.humdrumFilterDefinitions).map(([key, { command }]) => [
                    key,
                    typeof command === 'function' ? command(state) : command,
                ]),
            );
        },
        humdrumFilters(state) {
            return Object.entries(this.humdrumFilterDefinitions)
                .filter(([key]) => state[key])
                .sort(([, a], [, b]) => (b.priority ?? 0) - (a.priority ?? 0))
                .map(([key]) => this.humdrumFilterMap[key]);
        },
        verovioOptions: (state) => ({
            scale: state.verovioScale,
        }),
        countHumdrumFilters(state) {
            return Object.keys(this.humdrumFilterDefinitions).filter((key) => state[key]).length;
        },
        countOthers(state) {
            return [
                state.showHorizontalViewMode !== createDefaults().showHorizontalViewMode,
            ].filter(Boolean).length;
        },
        countTotal() {
            return this.countHumdrumFilters + this.countOthers;
        },
    },

    actions: {
        reset() {
            this.$patch(createDefaults());
        },
        zoomIn() {
            this.verovioScale = Math.min(this.verovioScale + 5, 100);
        },
        zoomOut() {
            this.verovioScale = Math.max(this.verovioScale - 5, 20);
        },
        resetZoom() {
            this.verovioScale = createDefaults().verovioScale;
        },
        resetHumdrumFilters() {
            const defaults = createDefaults();
            for (const key of Object.keys(this.humdrumFilterDefinitions)) {
                this[key] = defaults[key];
            }
        },
        resetVerovio() {
            const defaults = createDefaults();
            this.verovioScale = defaults.verovioScale;
            this.showHorizontalViewMode = defaults.showHorizontalViewMode;
        },
    },
});

export const useScoreOptions = defineStore('score_options', scoreOptionsStore(createDefaultScoreOptions));

export const useSearchScoreOptions = defineStore('search_score_options', scoreOptionsStore(() => ({
    ...createDefaultScoreOptions(),
    verovioScale: 35,
    showHorizontalViewMode: true,
})));

if (import.meta.hot) {
    import.meta.hot.accept(acceptHMRUpdate(useScoreOptions, import.meta.hot));
    import.meta.hot.accept(acceptHMRUpdate(useSearchScoreOptions, import.meta.hot));
}
