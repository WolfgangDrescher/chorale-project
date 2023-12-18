import { defineStore } from 'pinia';

const defaultFilter = {
    scoreFilters: [],
};

export const useBachViewOptionsStore = defineStore('bach_view_options', {
    state: () => ({ ...defaultFilter }),
});

export const useKittelViewOptionsStore = defineStore('kittel_view_options', {
    state: () => ({ ...defaultFilter }),
});

export const useSchiorringViewOptionsStore = defineStore('schiorring_view_options', {
    state: () => ({ ...defaultFilter }),
});

export const useScoreViewOptionsStore = defineStore('score_view_options', {
    state: () => ({
        scale: 40,
    }),
});
