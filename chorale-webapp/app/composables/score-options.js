import { defineStore, acceptHMRUpdate } from 'pinia';

// Filters every score goes through while their option is off, unless its view passes baseFilters.
export const BASE_FILTERS_BY_OPTION = {
    showLyrics: 'extract -I text',
};
export const BASE_FILTERS = Object.values(BASE_FILTERS_BY_OPTION);

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
        showModulations: false,
        showLyrics: false,
    };
}

// The Humdrum filters the score can be shown through, by the name of the option that
// toggles them. A filter with a higher priority is applied first, so the score is
// transposed and its voices are extracted before the remaining spines are analyzed. The
// command of an option with a value (not true or false) is made from the state.
export const CHORALE_HUMDRUM_FILTERS = {
    transposeKey: { command: (state) => `transpose -k ${state.transposeKey}`, priority: 3 },
    showMeter: { command: 'meter -f' },
    satb2gs: { command: 'satb2gs', priority: 1 },
    intervallsatz: { command: 'fb -icatm', priority: -1 },
    extendedFiguredbass: { command: 'fb -acon3' },
    bassScaleDegree: { command: 'deg --circle -k 1' },
    scaleDegree: { command: 'deg --circle' },
    // -k counts only **kern spines, so $ is the soprano and not its **text.
    hideMiddleVoices: { command: 'extract -k 1,$', priority: 2 },
    extractCantusFirmus: { command: 'extract -k $', priority: 2 },
    hideNonCantusFirmusVoices: {
        command: 'shed -s 1-3 -e "s/.*/$0yy/D s/.*//L s/^[^I].*//I"',
        priority: 2,
    },
};

// The zoom and the horizontal view, apart from the score options, so that stores can share them.
const scoreViewStore = (createDefaults) => ({
    state: () => createDefaults(),
    getters: {
        verovioOptions: (state) => ({
            scale: state.verovioScale,
            // Room between the verses, for the marks of a match of the lyrics.
            lyricHeightFactor: 1.4,
        }),
        isHorizontalViewModeChanged: (state) => state.showHorizontalViewMode !== createDefaults().showHorizontalViewMode,
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
    },
});

// Shared by the pages of all chorales.
export const useScoreViewOptions = defineStore('score_view_options', scoreViewStore(() => ({
    verovioScale: 40,
    showHorizontalViewMode: false,
})));

export const useSearchScoreViewOptions = defineStore('search_score_view_options', scoreViewStore(() => ({
    verovioScale: 35,
    showHorizontalViewMode: true,
})));

// The score options of a store with its own defaults and filters, so the search results do not
// share the options of the page of a chorale. The view comes from useView.
export const scoreOptionsStore = ({ createDefaults, humdrumFilterDefinitions, useView }) => ({
    state: () => createDefaults(),
    getters: {
        humdrumFilterDefinitions: () => humdrumFilterDefinitions,
        view: () => useView(),
        verovioOptions() {
            return this.view.verovioOptions;
        },
        showHorizontalViewMode() {
            return this.view.showHorizontalViewMode;
        },
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
        baseFilters: (state) => Object.entries(BASE_FILTERS_BY_OPTION)
            .filter(([option]) => option in state && !state[option])
            .map(([, filter]) => filter),
        countHumdrumFilters(state) {
            return Object.keys(this.humdrumFilterDefinitions).filter((key) => state[key]).length;
        },
        countOthers(state) {
            const defaults = createDefaults();
            const changed = ['showModulations', 'showLyrics'].filter((option) => option in state && state[option] !== defaults[option]);
            return changed.length + (this.view.isHorizontalViewModeChanged ? 1 : 0);
        },
        countTotal() {
            return this.countHumdrumFilters + this.countOthers;
        },
    },

    actions: {
        reset() {
            this.$patch(createDefaults());
            this.view.reset();
        },
        zoomIn() {
            this.view.zoomIn();
        },
        zoomOut() {
            this.view.zoomOut();
        },
        resetZoom() {
            this.view.resetZoom();
        },
        toggleHorizontalViewMode() {
            this.view.showHorizontalViewMode = !this.view.showHorizontalViewMode;
        },
        resetHumdrumFilters() {
            const defaults = createDefaults();
            for (const key of Object.keys(this.humdrumFilterDefinitions)) {
                this[key] = defaults[key];
            }
        },
        resetVerovio() {
            this.view.reset();
        },
    },
});

export const useScoreOptions = defineStore('score_options', scoreOptionsStore({
    createDefaults: createDefaultScoreOptions,
    humdrumFilterDefinitions: CHORALE_HUMDRUM_FILTERS,
    useView: useScoreViewOptions,
}));

export const useSearchScoreOptions = defineStore('search_score_options', scoreOptionsStore({
    createDefaults: createDefaultScoreOptions,
    humdrumFilterDefinitions: CHORALE_HUMDRUM_FILTERS,
    useView: useSearchScoreViewOptions,
}));

if (import.meta.hot) {
    import.meta.hot.accept(acceptHMRUpdate(useScoreOptions, import.meta.hot));
    import.meta.hot.accept(acceptHMRUpdate(useSearchScoreOptions, import.meta.hot));
    import.meta.hot.accept(acceptHMRUpdate(useScoreViewOptions, import.meta.hot));
    import.meta.hot.accept(acceptHMRUpdate(useSearchScoreViewOptions, import.meta.hot));
}
