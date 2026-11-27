import { patternPositionSchema, searchQueryDefinitions, searchRequestFieldSchemas, singleQuerySchema } from './chorale-search-query-schema.js';

// Schema of a group query for the editor, following GroupingJson.cpp.

const hintPairVoices = {
    'hint-12': 'bass and tenor',
    'hint-13': 'bass and alto',
    'hint-14': 'bass and soprano',
    'hint-23': 'tenor and alto',
    'hint-24': 'tenor and soprano',
    'hint-34': 'alto and soprano',
};

const readFeatureDescriptions = {
    'deg': 'The scale degree in the local key.',
    'mint': 'The melodic interval into the note.',
    'mint.direction': 'The direction of the melodic interval: +, - or none for a unison.',
    'mint.number': 'The number of the melodic interval, without direction and quality.',
    'mint.interval': 'The melodic interval without its direction.',
    'metweight': 'The metric weight of the note.',
    'duration': 'The duration of the note.',
    ...Object.fromEntries(Object.entries(hintPairVoices).map(([pair, voices]) => [pair, `The interval between ${voices}.`])),
    ...Object.fromEntries(Object.entries(hintPairVoices).map(([pair, voices]) => [`${pair}.number`, `The number of the interval between ${voices}.`])),
};

const notesOfVoiceReadFeatures = ['mint', 'mint.direction', 'mint.number', 'mint.interval', 'deg'];

const readFeatureSchema = {
    type: 'string',
    enum: Object.keys(readFeatureDescriptions),
    enumDescriptions: Object.values(readFeatureDescriptions),
    description: 'What is read: deg, mint, metweight, duration or a voice pair such as hint-14, or a part of mint or of a voice pair such as mint.direction.',
};

const countSchema = (description) => ({ type: 'integer', minimum: 0, description });

const categoriesSchema = {
    type: 'object',
    minProperties: 1,
    description: 'The values are put into categories, by name. A category is a list of values; the string "remaining" makes a category of every value in no list, at most once. A value in two lists is rejected.',
    additionalProperties: {
        oneOf: [
            { type: 'array', minItems: 1, items: { type: 'string' } },
            { type: 'string', const: 'remaining', description: 'Every value in no list.' },
        ],
    },
};

const readSchema = {
    type: 'object',
    required: ['feature'],
    additionalProperties: false,
    properties: {
        feature: readFeatureSchema,
        positions: {
            type: 'array',
            minItems: 1,
            items: { type: 'integer', minimum: 0 },
            description: 'The positions of the pattern where it is read, from 0, in the voice searched; all positions where it is left out. A note answering several positions is read once.',
        },
        notesOfVoice: {
            type: 'string',
            description: 'Read every note this voice attacks across the match instead, in its own rhythm: a single voice, such as "bass". Only with mint, a part of it, or deg, and not with positions.',
        },
        voice: {
            type: 'string',
            description: 'Read the scale degree this voice sounds at each position instead, also a note it holds: a single voice, such as "bass". Only with deg, and not with notesOfVoice.',
        },
        ignoreQuality: {
            type: 'boolean',
            description: '"-M2" and "-m2" count as "-2"; a unison keeps its quality. For mint and voice pairs.',
        },
        mintAllowIntervalComplementation: {
            type: 'boolean',
            description: 'An interval counts as its complement: "-P5" is "+P4". Only with notesOfVoice and mint.',
        },
        categories: categoriesSchema,
    },
    allOf: [
        {
            if: { required: ['notesOfVoice'] },
            then: {
                properties: {
                    feature: { enum: notesOfVoiceReadFeatures, description: 'notesOfVoice reads the feature mint, a part of it, or deg.' },
                    positions: { not: {}, description: 'notesOfVoice takes no positions.' },
                },
            },
        },
        {
            if: { required: ['voice'] },
            then: {
                properties: {
                    feature: { const: 'deg', description: 'voice reads the feature deg.' },
                    notesOfVoice: { not: {}, description: 'voice and notesOfVoice exclude each other.' },
                },
            },
        },
        {
            if: { required: ['mintAllowIntervalComplementation'], properties: { mintAllowIntervalComplementation: { const: true } } },
            then: { required: ['notesOfVoice'] },
        },
    ],
};

const levelSchema = {
    type: 'object',
    required: ['reads'],
    additionalProperties: false,
    properties: {
        reads: {
            type: 'array',
            minItems: 1,
            items: readSchema,
            description: 'What the level reads at every match. The values of all its reads together make a group.',
        },
        maxGroupCount: countSchema('Keep this many groups, the most frequent.'),
        minMatchCount: countSchema('Keep only groups with at least this many matches (default 1).'),
    },
};

const variantSchema = {
    type: 'object',
    additionalProperties: false,
    properties: {
        ignoreQuality: {
            type: 'boolean',
            description: 'Every read of every level reads without the quality.',
        },
        pattern: {
            type: 'array',
            items: { ...patternPositionSchema, minProperties: 0 },
            description: 'Keys added to the pattern of the query, position by position: [{}, { "phrase": "end" }] adds "phrase" to the second position. At most as long as the pattern.',
        },
    },
};

const groupQueryQuerySchema = {
    ...singleQuerySchema,
    description: 'A query of the search. Its matches are what is counted.',
    suggestSortText: '0',
    properties: {
        ...singleQuerySchema.properties,
        // No sets of voice pairs (hint-**).
        feature: searchRequestFieldSchemas.feature,
    },
};

export const choraleGroupQuerySchema = {
    $schema: 'http://json-schema.org/draft-07/schema#',
    title: 'Chorale Group Query',
    definitions: searchQueryDefinitions,
    type: 'object',
    required: ['query', 'groupBy'],
    additionalProperties: false,
    properties: {
        query: groupQueryQuerySchema,
        groupBy: {
            type: 'array',
            minItems: 1,
            items: levelSchema,
            description: 'The levels of the grouping. The first groups all matches, every further level groups each group of the level before it again.',
            suggestSortText: '1',
        },
        examples: {
            type: 'object',
            additionalProperties: false,
            description: 'The passages of the score shown as examples of each group.',
            suggestSortText: '2',
            properties: {
                maxCount: countSchema('How many examples each group shows at most; 0 for none.'),
            },
        },
        variants: {
            type: 'object',
            additionalProperties: variantSchema,
            description: 'The same grouping read another way, or of fewer matches, each under a name of its own.',
            suggestSortText: '3',
        },
    },
};
