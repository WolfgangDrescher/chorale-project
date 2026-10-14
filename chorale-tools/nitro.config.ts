import { defineNitroConfig } from 'nitropack/config';

const webappServer = '../chorale-webapp/server';

// This service runs the command line tools of chorale-search for the webapp
// (see the Docker setup). It serves the very endpoints of the webapp, so the
// requests, the validation and the errors are the same with and without it.
export default defineNitroConfig({
    // Keep this the same as in chorale-webapp/nuxt.config.ts.
    compatibilityDate: '2025-07-15',
    // Only these endpoints run here. An endpoint that should run in this
    // service has to be added by hand.
    handlers: ['chorale-search', 'chorale-segment', 'chorale-check'].map((name) => ({
        route: `/api/${name}`,
        handler: `${webappServer}/api/${name}.js`,
    })),
    // What the endpoints use without importing it (runCliTool, the errors,
    // ...). Only this file of the webapp's utils, since the others bring in the
    // PDF and MEI export.
    imports: { dirs: [`${webappServer}/utils/cli-endpoint.js`] },
});
