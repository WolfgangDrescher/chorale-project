import { defineCollection, defineContentConfig, z } from '@nuxt/content';

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
