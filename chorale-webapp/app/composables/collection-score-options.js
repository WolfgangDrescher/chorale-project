import { defineStore } from 'pinia';
import { CHORALE_HUMDRUM_FILTERS, scoreOptionsStore, useScoreViewOptions } from './score-options.js';

// The score options of a collection: its own filters, its filter groups and the shared view.

const COLLECTION_HUMDRUM_FILTERS = {
    modernClefs: { command: 'modori -m', priority: 1 },
    hideFiguredBass: { command: 'extract -I fb', priority: 1 },
    figuresWithoutSlashes: { command: 'shed -x fb -e "s/slash/Xslash/I"' },
    showMeter: CHORALE_HUMDRUM_FILTERS.showMeter,
    bassScaleDegree: CHORALE_HUMDRUM_FILTERS.bassScaleDegree,
    scaleDegree: CHORALE_HUMDRUM_FILTERS.scaleDegree,
};

const collectionScoreOptionStores = {};

export function useCollectionScoreOptions(collection) {
    collectionScoreOptionStores[collection.id] ??= defineStore(`score_options_${collection.id}`, scoreOptionsStore({
        createDefaults: () => ({
            ...Object.fromEntries(collection.scoreFilters.map((option) => [option, false])),
            // The ids of the options chosen in each filter group, or null for all of them.
            filterGroups: Object.fromEntries(collection.filterGroups.map(({ id }) => [id, null])),
        }),
        humdrumFilterDefinitions: Object.fromEntries(collection.scoreFilters.map((option) => [option, COLLECTION_HUMDRUM_FILTERS[option]])),
        useView: useScoreViewOptions,
    }));
    return collectionScoreOptionStores[collection.id]();
}

// The filters of the groups with options chosen.
export function filterGroupCommands(collection, chosenIds) {
    return collection.filterGroups.flatMap((group) => {
        const options = group.options.filter(({ id }) => chosenIds[group.id]?.includes(id));
        return options.length ? [group.command(options)] : [];
    });
}
