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
        verovioScale: 40,
        showHorizontalViewMode: false,
    };
}

export const useScoreOptions = defineStore('score_options', {
    state: () => createDefaultScoreOptions(),
    getters: {
        // The Humdrum filters the score can be shown through, by the name of the option that
        // toggles them. A filter with a higher priority is applied first, so voices are
        // extracted before the remaining spines are analyzed.
        humdrumFilterDefinitions: () => ({
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
        humdrumFilterMap() {
            return Object.fromEntries(
                Object.entries(this.humdrumFilterDefinitions).map(([key, { command }]) => [key, command]),
            );
        },
        humdrumFilters(state) {
            return Object.entries(this.humdrumFilterDefinitions)
                .filter(([key]) => state[key])
                .sort(([, a], [, b]) => (b.priority ?? 0) - (a.priority ?? 0))
                .map(([, { command }]) => command);
        },
        verovioOptions: (state) => ({
            scale: state.verovioScale,
        }),
        countHumdrumFilters(state) {
            return Object.keys(this.humdrumFilterDefinitions).filter((key) => state[key]).length;
        },
        countOthers(state) {
            return [
                state.showHorizontalViewMode,
            ].filter(Boolean).length;
        },
        countTotal() {
            return this.countHumdrumFilters + this.countOthers;
        },
    },

    actions: {
        reset() {
            this.$patch(createDefaultScoreOptions());
        },
        zoomIn() {
            this.verovioScale = Math.min(this.verovioScale + 5, 100);
        },
        zoomOut() {
            this.verovioScale = Math.max(this.verovioScale - 5, 20);
        },
        resetZoom() {
            this.verovioScale = createDefaultScoreOptions().verovioScale;
        },
        resetHumdrumFilters() {
            const defaults = createDefaultScoreOptions();
            for (const key of Object.keys(this.humdrumFilterDefinitions)) {
                this[key] = defaults[key];
            }
        },
        resetVerovio() {
            const defaults = createDefaultScoreOptions();
            this.verovioScale = defaults.verovioScale;
            this.showHorizontalViewMode = defaults.showHorizontalViewMode;
        },
    },
});

if (import.meta.hot) {
    import.meta.hot.accept(acceptHMRUpdate(useScoreOptions, import.meta.hot));
}
