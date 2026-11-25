import { readFile, writeFile } from 'node:fs/promises';

// The diminutions annotated by hand, which the tests of chorale-search check the queries against.
const DIMINUTIONS_FILE = '../annotations/bach-diminutions.json';

const POSITION = /^\d+\/\d+(\+\d+\/\d+)?$/;
const VOICES = ['bass', 'tenor', 'alto', 'soprano'];

// ["3/2+1/2", "alto"] or {"voice": "tenor", "preparation": "3/4", ...}.
function isAnnotation(annotation) {
    if (Array.isArray(annotation)) return annotation.length === 2 && POSITION.test(annotation[0]) && VOICES.includes(annotation[1]);
    if (!annotation || typeof annotation !== 'object') return false;
    const { voice, ...positionsByRole } = annotation;
    return VOICES.includes(voice) && Object.values(positionsByRole).length > 0 && Object.values(positionsByRole).every((position) => POSITION.test(position));
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
