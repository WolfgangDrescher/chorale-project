import { defineStore, acceptHMRUpdate } from 'pinia';

function createDefaultScoreOptions() {
    return {
        showMeter: false,
        bassstufen: false,
        hideFiguredbass: false,
        showFiguredbassAbove: false,
        showCadences: false,
        showSequences: false,
        showModulations: false,
        showModulationsDegLabel: false,
        hideInstrumentNames: false,
        showIntervallsatz: false,
        verovioScale: 40,
        showHorizontalViewMode: false,
    };
}

export const useScoreOptions = defineStore('score_options', {
    state: () => createDefaultScoreOptions(),
    getters: {
        humdrumFilterMap: () => ({
            showMeter: 'meter -f',
            bassstufen: 'deg -k1 --box -t',
            hideFiguredbass: 'extract -I "**fb" | extract -I "**fba"',
            showFiguredbassAbove: 'shed -e "s/fb/fba/gX"',
            hideInstrumentNames: 'shed -e "s/^I.*//gI"',
            showIntervallsatz: 'extract -I "**fb" | fb -catm --above | fb -b2 -k3 -catm --above',
        }),
        humdrumFilters(state) {
            const map = this.humdrumFilterMap;
            return Object.entries(map)
                .filter(([key]) => state[key])
                .map(([, value]) => value);
        },
        verovioOptions: (state) => ({
            scale: state.verovioScale,
        }),
        countHumdrumFilters(state) {
            const map = this.humdrumFilterMap;
            return Object.keys(map).filter((key) => state[key]).length;
        },
        countHighlights(state) {
            return [
                state.showCadences,
                state.showModulations,
                state.showModulationsDegLabel,
                state.showSequences,
            ].filter(Boolean).length;
        },
        countOthers(state) {
            return [
                state.showHorizontalViewMode,
            ].filter(Boolean).length; 
        },
        countTotal() {
            return this.countHumdrumFilters + this.countHighlights + this.countOthers;
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
            const map = this.humdrumFilterMap;
            for (const key of Object.keys(map)) {
                this[key] = defaults[key];
            }
        },
        resetVerovio() {
            const defaults = createDefaultScoreOptions();
            this.verovioScale = defaults.verovioScale;
            this.showHorizontalViewMode = defaults.showHorizontalViewMode;
        },
        resetHighlights() {
            const defaults = createDefaultScoreOptions();
            this.showCadences = defaults.showCadences;
            this.showSequences = defaults.showSequences;
            this.showModulations = defaults.showModulations;
            this.showModulationsDegLabel = defaults.showModulationsDegLabel;
        },
    },
});

if (import.meta.hot) {
    import.meta.hot.accept(acceptHMRUpdate(useScoreOptions, import.meta.hot));
}
