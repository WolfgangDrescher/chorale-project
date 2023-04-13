// https://nuxt.com/docs/api/configuration/nuxt-config
export default defineNuxtConfig({
    app: {
        buildAssetsDir: '/build/',
        head: {
            title: 'Chorale Project',
            meta: [
                { charset: 'utf-8' },
                { name: 'viewport', content: 'width=device-width, initial-scale=1' },
                { hid: 'robots', name: 'robots', content: process.env.DEPLOY_ENV === 'prod' ? 'all' : 'noindex' },
            ],
            link: [
                { rel: 'icon', type: 'image/svg+xml', href: '/favicon.svg', sizes: 'any' },
            ],
        },
    },
    ssr: true,
    srcDir: 'src/',
    modules: [
        '@nuxtjs/tailwindcss',
        '@nuxtjs/google-fonts',
        '@nuxtjs/i18n-edge',
        '@nuxt/content',
        'nuxt-icon',
    ],
    vite: {
        worker: {
            format: 'es',
        },
    },
    googleFonts: {
        stylePath: 'css/fonts.css',
        download: true,
        preload: true,
        prefetch: true,
        preconnect: true,
        display: 'swap',
        families: {
            Alegreya: {
                wght: [400, 700],
                ital: [400],
            },
            'Alegreya Sans': {
                wght: [400, 700, 800],
            },
        },
    },
    i18n: {
        strategy: 'prefix_except_default',
        locales: [
            { code: 'de', iso: 'de-DE', file: 'de.yaml', dir: 'ltr' },
        ],
        defaultLocale: 'de',
        langDir: 'locales/',
        config: {
            fallbackLocale: 'de',
            legacy: false,
            locale: 'en',
        },
    },
});
