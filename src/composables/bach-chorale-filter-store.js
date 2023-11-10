import { defineStore } from 'pinia';

const defaultFilter = {
    searchText: null,
    keys: [],
    majorMinor: null,
    countCadences: [0, 23],
    numberOfMeasures: [0, 48],
    timeSignature: null,
    cadenceDegrees: [],
    cadenceDegreeFbNumbers: [],
};

export const useBachChoraleFilterStore = defineStore('bach_chorale_filter', {
    state: () => ({ ...defaultFilter }),
    actions: {
        update(prop, value) {
            this[prop] = value;
        },
        reset() {
            this.$patch({ ...defaultFilter });
        },
    },
});
