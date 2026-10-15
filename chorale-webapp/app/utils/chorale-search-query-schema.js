// One comparison, an entry of a pattern value list like any string: the score's value is
// compared with `value` instead of matched against it. A list is an OR, so a range ("at least
// an eighth and at most a quarter") is written as the negation of both ends instead.
const comparisonSchema = {
    type: 'object',
    required: ['operator', 'value'],
    additionalProperties: false,
    properties: {
        operator: {
            // The editor suggests the spelled-out names. The abbreviations and the symbols the backend also
            // accepts (gt, >, ...) are valid too, but are not offered.
            anyOf: [
                { type: 'string', enum: ['greaterThan', 'greaterThanOrEqual', 'lessThan', 'lessThanOrEqual'] },
                { type: 'string', pattern: '^(gte?|lte?|>=?|<=?)$' },
            ],
            description: 'greaterThan, greaterThanOrEqual, lessThan or lessThanOrEqual. For kern the greater note is the higher one, for duration the longer one, for mint and hint-<pair>/hint-<voice> the wider interval.',
        },
        value: {
            type: 'string',
            description: 'What to compare with: a pitch for "kern" (e.g. "g", "f#"), a recip duration for "duration" (e.g. "4", "4."), and for "mint" and the hint keys the size of an interval: a diatonic number ("3" compares the number alone, so any third is as wide as any other) or a number with a quality ("m3", "A4", "M10" compare the size in semitones). "mint" may also carry a sign ("+3", "-m6") to compare in that direction only. Only kern, duration, mint and the hint keys can be compared. For kern and duration it can also be a voice reference ("@2", "@below", "@anyBelow", ...): the note another voice sounds at the same time.',
        },
    },
};

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
        description: 'Which voices to search: "all", a number 1-4, a name/initial (bass/tenor/alto/soprano, b/t/a/s), a run of digits, or a comma-separated mix. For a query over voice pairs (hint-**), "@lower" and "@upper" are the lower and the upper voice of the pair being searched, and the query itself reads all voices (each pair once) or "@lower" and "@upper" only.',
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
                '^!?(kern|deg|mint|fb|metweight|duration|fermata|phrase|hint-(?:[1-4*][1-4*]|[1-4]|@pair))$': {
                    description: 'An OR-list of acceptable values for this feature at this position (or a single value). A value is a string, or for kern, duration, mint and the hint keys a comparison ({ "operator": "gt", "value": "4" }). For kern and duration a value may also be a voice reference ("@2", "@below", "@anyAbove", ...), the note another voice sounds at the same time. Prefix the key with "!" to negate the whole position.',
                    oneOf: [
                        { type: 'string' },
                        { type: 'boolean' },
                        comparisonSchema,
                        { type: 'array', items: { oneOf: [{ type: 'string' }, comparisonSchema] }, minItems: 1 },
                    ],
                },
            },
            propertyNames: {
                pattern: '^!?(kern|deg|mint|fb|metweight|duration|fermata|phrase|hint-(?:[1-4*][1-4*]|[1-4]|@pair))$',
                description: 'A feature to check at this position, optionally prefixed with "!" to negate the whole position. hint-<pair> (e.g. hint-14) names a fixed pair, optionally with "*" for either digit (e.g. hint-*4); hint-@pair is the pair a query over voice pairs is searching; hint-<voice> (e.g. hint-2) is relative to whichever voice is currently being walked.',
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
    durationAllowSplitNotes: {
        type: 'boolean',
        default: true,
        description: 'Affects any "duration" pattern key: lets a position also be satisfied by consecutive onsets that re-articulate the same note and whose durations add up to one of its values, so e.g. "2" also matches two repeated quarters.',
        suggestSortText: '6.7',
    },
    durationAllowMergedNotes: {
        type: 'boolean',
        default: true,
        description: 'Affects any "duration" pattern key: lets a single onset also satisfy several consecutive positions whose durations add up to exactly its own, so e.g. two quarters also match one half note.',
        suggestSortText: '6.8',
    },
    metweightSkipUnclassified: {
        type: 'boolean',
        default: true,
        description: 'Treats an onset on a metrically unclassified position ("metweight": "u") as an ornament rather than a note of its own: the pattern never sees it, its duration falls to the note it decorates, and the next note\'s "mint" is measured across it. Lets one pattern spelling out a plain skeleton find the diminished readings of it too.',
        suggestSortText: '6.9',
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

// Which chorales a query searches: one entry of a list of ids is a number or a string, and a count is a
// number or a range. Only the top-level query has one, a simultaneousWith group runs in the same chorale.
const choraleIdSchema = {
    oneOf: [
        { type: 'integer', minimum: 0 },
        {
            type: 'string',
            pattern: '^([cC][hH][oO][rR])?\\d+(-([cC][hH][oO][rR])?\\d+)?$',
            description: 'A chorale by its number ("29", "029" or "chor029") or a range of them ("5-20", "chor005-chor020").',
        },
    ],
};

const choraleIdsSchema = (description) => ({
    description,
    oneOf: [choraleIdSchema, { type: 'array', minItems: 1, items: choraleIdSchema }],
});

const stringsSchema = (itemSchema, description) => ({
    description,
    oneOf: [itemSchema, { type: 'array', minItems: 1, items: itemSchema }],
});

const countSchema = (description) => ({
    description,
    oneOf: [
        { type: 'integer', minimum: 0 },
        {
            type: 'object',
            minProperties: 1,
            additionalProperties: false,
            properties: {
                min: { type: 'integer', minimum: 0 },
                max: { type: 'integer', minimum: 0 },
            },
        },
    ],
});

const choraleFilterSchema = {
    type: 'object',
    description: 'Which chorales are searched. A chorale is searched only if every field that is set fits it; a list is an OR.',
    additionalProperties: false,
    suggestSortText: '2.5',
    properties: {
        ids: choraleIdsSchema('Only these chorales, by the number in their id: 29, "029", "chor029", or a range such as "5-20".'),
        excludeIds: choraleIdsSchema('All chorales but these, written like ids.'),
        title: stringsSchema({ type: 'string', minLength: 1 }, 'Text the German or English title contains, without regard to case.'),
        meter: stringsSchema({ type: 'string', pattern: '^\\d+/\\d+$' }, 'A meter the chorale is in at some point, e.g. "4/4".'),
        measureCount: countSchema('The number of measures, a pickup not counted: a number or { "min": 8, "max": 16 }.'),
        phraseCount: countSchema('The number of phrases, which the fermatas delimit: a number or { "min": 4 }.'),
        startKey: stringsSchema({ type: 'string', pattern: '^[a-gA-G][#-]*$' }, 'The key the chorale starts in, not one it modulates to later. The key is written as in the score: "G" is G major, "e" is E minor, "B-" is B-flat major.'),
        startMode: stringsSchema(
            { type: 'string', enum: ['major', 'minor', 'ionian', 'aeolian', 'dorian', 'phrygian', 'lydian', 'mixolydian', 'locrian'] },
            'The mode the chorale starts in, not one it modulates to later. ionian is interpreted as major and aeolian as minor.',
        ),
    },
};

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
        feature: {
            ...searchRequestFieldSchemas.feature,
            enum: [...searchRequestFieldSchemas.feature.enum, 'hint-**', 'hint-1*', 'hint-2*', 'hint-3*', 'hint-*2', 'hint-*3', 'hint-*4'],
            description: searchRequestFieldSchemas.feature.description + ' A set of voice pairs, hint-** (every pair) or one with a voice in its place, like hint-*4 (the soprano above another voice) or hint-1* (the bass below another voice), searches the pairs one after the other: hint-@pair in the pattern is the interval of the pair being searched, and every result has its pair as feature, e.g. hint-24.',
        },
        limit: {
            type: 'integer',
            minimum: 0,
            default: 100,
            description: 'Stop after this many results, counted across the whole corpus.',
            suggestSortText: '3',
        },
        choraleFilter: choraleFilterSchema,
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
