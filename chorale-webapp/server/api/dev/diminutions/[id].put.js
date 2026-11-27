import { readFile, writeFile } from 'node:fs/promises';

// The diminutions annotated by hand, which the tests of chorale-search check the queries against.
const DIMINUTIONS_FILE = '../annotations/bach-diminutions.json';

const POSITION = /^\d+\/\d+(\+\d+\/\d+)?$/;
const VOICES = ['bass', 'tenor', 'alto', 'soprano'];

// ["3/2+1/2", "alto"] or {"voice": "tenor", "preparation": "3/4", ...}, uncertain as ["3/2+1/2", "alto", "?"] or
// with "uncertain": true.
function isAnnotation(annotation) {
    if (Array.isArray(annotation)) {
        const [position, voice, uncertain] = annotation;
        const certainOrMarkedUncertain = annotation.length === 2 || (annotation.length === 3 && uncertain === '?');
        return certainOrMarkedUncertain && POSITION.test(position) && VOICES.includes(voice);
    }
    if (!annotation || typeof annotation !== 'object') return false;
    const { voice, uncertain, ...positionsByRole } = annotation;
    return VOICES.includes(voice) && [undefined, true].includes(uncertain)
        && Object.values(positionsByRole).length > 0 && Object.values(positionsByRole).every((position) => POSITION.test(position));
}

// Saves { diminutions: { "consonantLeap": [...], ... } } of a chorale.
export default defineEventHandler(async (event) => {
    if (!import.meta.dev) throw createError({ statusCode: 404, statusMessage: 'Page Not Found' });

    const { id } = getRouterParams(event);
    if (!/^chor\d{3}$/.test(id)) throw createError({ statusCode: 404, statusMessage: 'Page Not Found' });

    const { diminutions } = await readBody(event) ?? {};
    const isValid = diminutions && typeof diminutions === 'object' && Object.values(diminutions).every((entries) => Array.isArray(entries) && entries.every(isAnnotation));
    if (!isValid) {
        throw createError({ statusCode: 422, statusMessage: 'Unprocessable Content', message: '"diminutions" must hold a list of annotations for each diminution' });
    }

    const annotations = JSON.parse(await readFile(DIMINUTIONS_FILE, 'utf8'));
    annotations[id] = diminutions;
    await writeFile(DIMINUTIONS_FILE, formatDiminutionAnnotations(annotations));
    return { saved: true };
});
