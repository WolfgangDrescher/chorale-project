export const choraleSearchQuerySchema = {
    $schema: 'http://json-schema.org/draft-07/schema#',
    title: 'Chorale Search Query',
    type: 'object',
    required: ['feature', 'pattern'],
    additionalProperties: false,
    properties: {
        feature: {
            type: 'string',
            enum: ['kern', 'deg', 'mint', 'fb'],
            description: 'The driving feature: which analysis spine the search walks through.',
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
                    '^!?(kern|deg|mint|fb|duration|fermata)$': {
                        description: 'An OR-list of acceptable values for this feature at this position (or a single value). Prefix the key with "!" to negate the whole position.',
                        oneOf: [
                            { type: 'string' },
                            { type: 'boolean' },
                            { type: 'array', items: { type: 'string' }, minItems: 1 },
                        ],
                    },
                },
                propertyNames: {
                    enum: ['kern', 'deg', 'mint', 'fb', 'duration', 'fermata', '!kern', '!deg', '!mint', '!fb', '!duration', '!fermata'],
                    description: 'A feature to check at this position, optionally prefixed with "!" to negate the whole position.',
                },
            },
        },
        limit: {
            type: 'integer',
            minimum: 0,
            default: 100,
            description: 'Stop after this many results, counted across the whole corpus.',
            suggestSortText: '3',
        },
        mintStartAtPreviousToken: {
            type: 'boolean',
            default: true,
            description: 'Only relevant for feature "mint": shifts the reported match start back by one onset.',
            suggestSortText: '4',
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
    },
};
