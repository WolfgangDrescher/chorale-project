import { defineStore } from 'pinia';

const defaultFilter = {
    searchText: '',
    keys: [],
    majorMinor: '',
    countPhrases: [0, 23],
    numberOfMeasures: [0, 48],
    timeSignature: '',
    phraseDegrees: [],
    phraseFbNumbers: [],
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
