import { defineCollection, defineContentConfig, z } from '@nuxt/content';

// A key as chorale-info writes it: "E-", "major", its degree in the starting key and its signature.
const keySchema = z.object({
    key: z.string(),
    mode: z.enum(['major', 'minor', 'dorian', 'phrygian', 'lydian', 'mixolydian', 'locrian']),
    keyDeg: z.string(),
    signature: z.number(),
});

export default defineContentConfig({
    collections: {
        // One file per chorale, generated from the `make kern` corpus by
        // scripts/extract-chorales.mjs.
        chorales: defineCollection({
            source: 'chorales/**/*.yaml',
            type: 'page',
            schema: z.object({
                choraleId: z.string(),
                title: z.string().optional(),
                composer: z.string().optional(),
                bwv: z.string().optional(),
                number: z.number().optional(),
                key: z.string().optional(),
                mode: z.enum(['major', 'minor', 'ionian', 'dorian', 'phrygian', 'lydian', 'mixolydian', 'aeolian', 'locrian']).optional(),
                meter: z.string().optional(),
                duration: z.number().optional(),
                modulations: z.array(keySchema.extend({ line: z.number() })).optional(),
                cantusFirmus: keySchema.optional(),
            }),
        }),
        lessons: defineCollection({
            type: 'page',
            source: 'lessons/**/*.md',
            schema: z.object({
                icon: z.string().optional(),
                lessonNumber: z.number().optional(),
                queries: z.array(z.object({
                    id: z.string(),
                    title: z.string(),
                    query: z.string(),
                })).optional(),
                exercises: z.array(z.object({
                    type: z.enum(['composition', 'analysis', 'comparison']).optional(),
                    title: z.string(),
                    description: z.string(),
                    chorales: z.array(z.string()),
                    export: z.object({
                        formats: z.array(z.enum(['pdf', 'mei'])).optional(),
                        content: z.enum(['cantus-firmus', 'complete']).optional(),
                        layout: z.enum(['score', 'piano']).optional(),
                        voices: z.enum(['4', '2']).optional(),
                        orientation: z.enum(['portrait', 'landscape']).optional(),
                    }).optional(),
                    scoreFilters: z.array(z.string()).optional(),
                })).optional(),
                analyses: z.array(z.object({
                    name: z.string(),
                    title: z.string().optional(),
                    view: z.enum(['bars', 'stacked']).optional(),
                    colors: z.record(z.string(), z.string()).optional(),
                    labels: z.array(z.string()).optional(),
                })).optional(),
                analysesDescription: z.string().optional(),
            }),
        }),
        analysisResults: defineCollection({
            type: 'data',
            source: 'analysis-results/*.json',
            schema: z.object({
                matchCount: z.number(),
                choraleCount: z.number(),
                query: z.record(z.string(), z.any()),
                groups: z.array(z.any()),
                omittedMatchCount: z.number(),
                groupBy: z.array(z.any()),
                variants: z.record(z.string(), z.any()).optional(),
            }),
        }),
        changelog: defineCollection({
            type: 'page',
            source: 'changelog/*.md',
            schema: z.object({
                date: z.date(),
                lastCommit: z.string().optional(),
                commitCount: z.number().optional(),
                changes: z.array(z.object({
                    type: z.enum(['feature', 'improvement', 'fix']),
                    text: z.string(),
                    to: z.string().optional(),
                    commits: z.array(z.string()).optional(),
                    github: z.array(z.string().regex(/^([\w.-]+\/[\w.-]+)?#\d+$/)).optional(),
                })).optional(),
            }),
        }),
        docs: defineCollection({
            type: 'page',
            source: 'docs/**/*.md',
            schema: z.object({
                icon: z.string().optional(),
            }),
        }),
    },
});
