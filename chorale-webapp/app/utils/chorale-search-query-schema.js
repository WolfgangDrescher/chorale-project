// Fields shared by the top-level query and each simultaneousWith entry, defined once so
// both schemas below stay in sync.
const searchRequestFieldSchemas = {
    feature: {
        type: 'string',
        enum: ['kern', 'deg', 'mint', 'fb', 'metweight', 'hint-12', 'hint-13', 'hint-14', 'hint-23', 'hint-24', 'hint-34'],
        description: 'The driving feature: which analysis spine the search walks through. A hint-<pair> names a specific pair of voices (lower voice number first).',
        suggestSortText: '0',
    },
    voices: {
        type: 'string',
        default: 'all',
        description: 'Which voices to search: "all", a number 1-4, a name/initial (bass/tenor/alto/soprano, b/t/a/s), a run of digits, or a comma-separated mix.',
        suggestSortText: '1',
    },
    pattern: {
        type: 'array',
        minItems: 1,
        description: 'The sequence of positions to search for, one object per position.',
        suggestSortText: '2',
        items: {
            type: 'object',
            minProperties: 1,
            additionalProperties: false,
            patternProperties: {
                '^!?(kern|deg|mint|fb|metweight|duration|fermata|hint-(?:[1-4*][1-4*]|[1-4]))$': {
                    description: 'An OR-list of acceptable values for this feature at this position (or a single value). Prefix the key with "!" to negate the whole position.',
                    oneOf: [
                        { type: 'string' },
                        { type: 'boolean' },
                        { type: 'array', items: { type: 'string' }, minItems: 1 },
                    ],
                },
            },
            propertyNames: {
                pattern: '^!?(kern|deg|mint|fb|metweight|duration|fermata|hint-(?:[1-4*][1-4*]|[1-4]))$',
                description: 'A feature to check at this position, optionally prefixed with "!" to negate the whole position. hint-<pair> (e.g. hint-14) names a fixed pair, optionally with "*" for either digit (e.g. hint-*4); hint-<voice> (e.g. hint-2) is relative to whichever voice is currently being walked.',
            },
        },
    },
    mintStartAtPreviousToken: {
        type: 'boolean',
        default: true,
        description: 'Only relevant for feature "mint": shifts the reported match start back by one onset.',
        suggestSortText: '4',
    },
    mintAllowIntervalComplementation: {
        default: '*',
        description: 'Only relevant for the "mint" pattern key: the diatonic numbers (1-8, or "*" for all) whose pattern values may also be satisfied by their complementary interval, so e.g. ["5"] lets "-5" also match "+4".',
        suggestSortText: '4.5',
        oneOf: [
            { type: 'string', enum: ['1', '2', '3', '4', '5', '6', '7', '8', '*'] },
            { type: 'integer', minimum: 1, maximum: 8 },
            {
                type: 'array',
                minItems: 1,
                items: {
                    oneOf: [
                        { type: 'string', enum: ['1', '2', '3', '4', '5', '6', '7', '8', '*'] },
                        { type: 'integer', minimum: 1, maximum: 8 },
                    ],
                },
            },
        ],
    },
    fbCompareExactChord: {
        type: 'boolean',
        default: true,
        description: 'Only relevant for feature "fb": requires the actual chord to have exactly as many figures as the pattern value, no extras.',
        suggestSortText: '5',
    },
    kernIgnoreOctave: {
        type: 'boolean',
        default: true,
        description: 'Affects any "kern" pattern key: ignores register, so e.g. "G" matches every octave of that pitch class.',
        suggestSortText: '6',
    },
    hintReduceCompound: {
        type: 'boolean',
        default: true,
        description: 'Affects any "hint-<pair>"/"hint-<voice>" pattern key: folds both the pattern value and the actual interval to within an octave before comparing, so e.g. "3" also matches a 10th.',
        suggestSortText: '6.5',
    },
    simultaneousAlignment: {
        type: 'string',
        enum: ['start', 'end', 'start-end'],
        default: 'start',
        description: 'Which position of a simultaneousWith match must line up with the primary match: its start, its end, or both.',
        suggestSortText: '7',
    },
};

// mintStartAtPreviousToken shifts the reported start by one onset of the driving feature, which
// only means anything while walking mint itself -- the backend rejects `true` next to any other
// feature, so flag it here too instead of letting the editor suggest a query that can't run.
// Applies per object: a simultaneousWith group is judged by its own feature, not the query's.
const mintStartAtPreviousTokenOnlyForMint = [
    {
        if: {
            required: ['feature'],
            properties: { feature: { not: { const: 'mint' } } },
        },
        then: {
            properties: {
                mintStartAtPreviousToken: {
                    const: false,
                    description: 'mintStartAtPreviousToken can only be true when feature is "mint".',
                },
            },
        },
    },
];

const singleQuerySchema = {
    type: 'object',
    title: 'Chorale Search Query',
    required: ['feature', 'pattern'],
    additionalProperties: false,
    allOf: mintStartAtPreviousTokenOnlyForMint,
    properties: {
        id: {
            type: 'string',
            description: 'Only meaningful as an entry of a top-level array of queries: tags every result of this query with this id instead of its position in the array, so the caller can tell which query produced it.',
            suggestSortText: '-1',
        },
        ...searchRequestFieldSchemas,
        limit: {
            type: 'integer',
            minimum: 0,
            default: 100,
            description: 'Stop after this many results, counted across the whole corpus.',
            suggestSortText: '3',
        },
        simultaneousWith: {
            type: 'array',
            description: 'Additional patterns (typically in other voices) that must each have a match at the same musical position as a match of this query\'s own pattern, for that match to be kept.',
            suggestSortText: '8',
            items: {
                type: 'object',
                required: ['feature', 'pattern'],
                additionalProperties: false,
                allOf: mintStartAtPreviousTokenOnlyForMint,
                properties: searchRequestFieldSchemas,
            },
        },
    },
};

export const choraleSearchQuerySchema = {
    $schema: 'http://json-schema.org/draft-07/schema#',
    title: 'Chorale Search Query',
    oneOf: [
        singleQuerySchema,
        {
            type: 'array',
            minItems: 1,
            items: singleQuerySchema,
            description: 'Several queries combined into one search: each runs independently across the corpus, and every result is tagged with queryId (that query\'s own "id", or its position in the array if it didn\'t set one) so the combined results can be told apart.',
        },
    ],
};
