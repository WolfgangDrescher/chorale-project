// Bach's chorales: the corpus that is searched, generated into kern/ by `make kern`.
export const BACH_CHORALES = {
    id: 'bach',
    name: 'Bach',
    directory: 'bach-370-chorales',
    repository: 'craigsapp/bach-370-chorales',
    branch: 'main',
};

// The filter of a group that shows the **kern spines of the chosen options, as `extract -k` counts
// them, and the shared ones with every choice.
const extractKernSpines = (shared = []) => (options) => (
    `extract -k ${[...options.flatMap(({ kernSpines }) => kernSpines), ...shared].sort((a, b) => a - b).join(',')}`
);

// The collections shown besides Bach's chorales, one submodule each. They are not searched.
//
// scoreFilters: the filters of the palette, see useCollectionScoreOptions.
// filterGroups: filters chosen with buttons over the score. label is a translation key, so is
// the label of an option without a number. command makes the filter of the chosen options.
export const CHORALE_COLLECTIONS = [
    {
        id: 'kittel',
        name: 'Kittel',
        directory: 'kittel-24-chorales',
        repository: 'WolfgangDrescher/kittel-24-chorales',
        branch: 'master',
        titleRecord: 'OTL@@DE',
        scoreFilters: ['hideFiguredBass', 'figuresWithoutSlashes', 'bassScaleDegree', 'scaleDegree', 'showMeter'],
        filterGroups: [{
            id: 'bass',
            label: 'bass',
            icon: 'lucide:music-2',
            // Bass 8 on the left to bass 1, then the cantus firmus they share.
            options: Array.from({ length: 8 }, (_, index) => ({ id: `bass${index + 1}`, number: index + 1, kernSpines: [8 - index] })),
            command: extractKernSpines([9]),
        }],
    },
    {
        id: 'schiorring',
        name: 'Schiørring',
        directory: 'schiorring-choral-bog',
        repository: 'WolfgangDrescher/schiorring-choral-bog',
        branch: 'master',
        titleRecord: 'OTL@@DA',
        scoreFilters: ['modernClefs', 'hideFiguredBass', 'figuresWithoutSlashes', 'showMeter'],
        filterGroups: [{
            id: 'version',
            label: 'version',
            icon: 'lucide:layers',
            // *part6, *part5 and the four voices.
            options: [
                { id: 'figuredBass', label: 'figuredBassVersion', kernSpines: [1, 2] },
                { id: 'keyboard', label: 'keyboardVersion', kernSpines: [3, 4] },
                { id: 'satb', label: 'satbVersion', kernSpines: [5, 6, 7, 8] },
            ],
            command: extractKernSpines(),
        }],
    },
];

export function findChoraleCollection(id) {
    return [BACH_CHORALES, ...CHORALE_COLLECTIONS].find((collection) => collection.id === id) ?? null;
}
