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
