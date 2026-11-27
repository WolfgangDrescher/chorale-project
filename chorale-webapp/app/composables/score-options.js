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
        oldClefs: false,
        intervallsatz: false,
        extendedFiguredbass: false,
        bassScaleDegree: false,
        scaleDegree: false,
        dissonant: false,
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

// The results of a search do not show the modulations of their chorales.
function createDefaultSearchScoreOptions() {
    const options = createDefaultScoreOptions();
    delete options.showModulations;
    return options;
}

// The Humdrum filters the score can be shown through, by the name of the option that
// toggles them. A filter with a higher priority is applied first, so the score is
// transposed and its voices are extracted before the remaining spines are analyzed. The
// command of an option with a value (not true or false) is made from the state.
export const CHORALE_HUMDRUM_FILTERS = {
    transposeKey: { command: (state) => `transpose -k ${state.transposeKey}`, priority: 3 },
    showMeter: { command: 'meter -f' },
    satb2gs: { command: 'satb2gs', priority: 1 },
    oldClefs: {
        command: 'shed -s 2 -e "s/^clef.*/clefC4/I" | shed -s 3 -e "s/^clef.*/clefC3/I" | shed -s 4 -e "s/^clef.*/clefC1/I"',
        priority: 4,
    },
    intervallsatz: { command: 'fb -icatm', priority: -1 },
    extendedFiguredbass: { command: 'fb -acon3' },
    bassScaleDegree: { command: 'deg --circle -k 1' },
    scaleDegree: { command: 'deg --circle' },
    dissonant: { command: 'dissonant' },
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
    persist: true,
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
    persist: true,
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
            return changed.length + (this.view.isHorizontalViewModeChanged ? 1 : 0) + (state.showDiminutions ? 1 : 0);
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
        toggleDiminution(id) {
            this.diminutions = this.diminutions.includes(id)
                ? this.diminutions.filter((shown) => shown !== id)
                : [...this.diminutions, id];
            if (this.diminutions.includes(id)) this.showDiminutions = true;
        },
        // Shown without any chosen, all of them are.
        toggleDiminutions() {
            this.showDiminutions = !this.showDiminutions;
            if (this.showDiminutions && !this.diminutions.length) this.diminutions = DIMINUTIONS.map(({ id }) => id);
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
    createDefaults: () => ({
        ...createDefaultScoreOptions(),
        // Whether the diminutions are marked in the score, and which, see composables/diminutions.js. The choice
        // stays while they are hidden.
        showDiminutions: false,
        diminutions: [],
    }),
    humdrumFilterDefinitions: CHORALE_HUMDRUM_FILTERS,
    useView: useScoreViewOptions,
}));

export const useSearchScoreOptions = defineStore('search_score_options', scoreOptionsStore({
    createDefaults: createDefaultSearchScoreOptions,
    humdrumFilterDefinitions: CHORALE_HUMDRUM_FILTERS,
    useView: useSearchScoreViewOptions,
}));

if (import.meta.hot) {
    import.meta.hot.accept(acceptHMRUpdate(useScoreOptions, import.meta.hot));
    import.meta.hot.accept(acceptHMRUpdate(useSearchScoreOptions, import.meta.hot));
    import.meta.hot.accept(acceptHMRUpdate(useScoreViewOptions, import.meta.hot));
    import.meta.hot.accept(acceptHMRUpdate(useSearchScoreViewOptions, import.meta.hot));
}
