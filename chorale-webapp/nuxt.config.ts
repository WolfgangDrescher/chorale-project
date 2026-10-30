import { fileURLToPath } from 'node:url';

// https://nuxt.com/docs/api/configuration/nuxt-config
export default defineNuxtConfig({
    compatibilityDate: '2025-07-15',
    devtools: { enabled: false },
    modules: ['@nuxt/ui', '@nuxt/content', '@nuxtjs/i18n', '@pinia/nuxt', '@nuxtjs/plausible'],
    css: ['~/assets/css/main.css'],
    app: {
        head: {
            link: [
                { rel: 'icon', type: 'image/svg+xml', href: '/favicon.svg' },
                { rel: 'icon', type: 'image/x-icon', href: '/favicon.ico', sizes: '16x16 32x32 48x48' },
                { rel: 'apple-touch-icon', href: '/apple-touch-icon.png' },
            ],
        },
    },
    content: {
        experimental: { sqliteConnector: 'native' },
    },
    i18n: {
        strategy: 'prefix_except_default',
        locales: [
            { code: 'de', language: 'de-DE', files: ['de.yaml', 'de.lessons.yaml'], dir: 'ltr' },
        ],
        defaultLocale: 'de',
        langDir: 'locales/',
        // What the server translates in, e.g. the footer of a PDF (see i18n/localeDetector.js).
        experimental: { localeDetector: 'localeDetector.js' },
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
    // Plausible is off until the container switches it on, see compose.yaml. The empty host
    // replaces the default plausible.io, so nothing ever goes to a foreign instance.
    plausible: {
        enabled: false,
        apiHost: '',
    },
    nitro: {
        // The Verovio of the worker alone is about 12 MB.
        compressPublicAssets: true,
        routeRules: {
            '/kern/**': { prerender: false },
        },
        // The font of the PDF downloads (see server/utils/chorale-pdf.js). Only the files used there
        // are bundled, not the whole package (all weights, scripts and formats).
        serverAssets: [
            {
                baseName: 'fonts',
                dir: fileURLToPath(new URL('./node_modules/@fontsource/alegreya-sans/files', import.meta.url)),
                pattern: [
                    'alegreya-sans-latin-400-normal.woff',
                    'alegreya-sans-latin-400-italic.woff',
                    'alegreya-sans-latin-700-normal.woff',
                ],
            },
        ],
        publicAssets: [
            {
                baseURL: 'kern/bach-370-chorales',
                dir: fileURLToPath(new URL('../kern/bach-370-chorales', import.meta.url)),
                maxAge: 3600,
            },
        ],
    },
})