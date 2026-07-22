import { fileURLToPath } from 'node:url';

// https://nuxt.com/docs/api/configuration/nuxt-config
export default defineNuxtConfig({
    compatibilityDate: '2025-07-15',
    devtools: { enabled: false },
    modules: ['@nuxt/ui', '@nuxt/content', '@nuxtjs/i18n', '@pinia/nuxt'],
    css: ['~/assets/css/main.css'],
    i18n: {
        strategy: 'prefix_except_default',
        locales: [
            { code: 'de', language: 'de-DE', file: 'de.yaml', dir: 'ltr' },
        ],
        defaultLocale: 'de',
        langDir: 'locales/',
    },
    vite: {
        worker: {
            format: 'es',
        },
        optimizeDeps: {
            exclude: ['verovio'],
        },
    },
    colorMode: {
        preference: 'light',
    },
    nitro: {
        routeRules: {
            '/kern/**': { prerender: false },
        },
        publicAssets: [
            {
                baseURL: 'kern/bach-370-chorales',
                dir: fileURLToPath(new URL('../kern/bach-370-chorales', import.meta.url)),
                maxAge: 3600,
            },
        ],
    },
})