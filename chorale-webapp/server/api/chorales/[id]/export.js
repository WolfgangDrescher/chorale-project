// Downloads a chorale: /api/chorales/:id/export?format=mei&layout=score|piano
// See chorale-mei.js for what the options mean.
const FORMATS = ['mei'];

const CONTENT_TYPES = {
    mei: 'application/xml',
};

function invalidQuery(name, allowed) {
    return createError({
        statusCode: 422,
        statusMessage: 'Unprocessable Content',
        message: `"${name}" must be one of: ${allowed.join(', ')}`,
    });
}

export default defineEventHandler(async (event) => {
    const { id } = getRouterParams(event);
    const { format = 'mei', layout = 'piano' } = getQuery(event);

    if (!FORMATS.includes(format)) throw invalidQuery('format', FORMATS);
    if (!CHORALE_EXPORT_LAYOUTS.includes(layout)) throw invalidQuery('layout', CHORALE_EXPORT_LAYOUTS);

    let kern;
    try {
        kern = await loadChoraleKern(id);
    } catch {
        throw createError({ statusCode: 404, statusMessage: 'Page Not Found' });
    }

    const filters = choraleFilters({ layout });
    const prepared = restOtherVoices(kern);

    setResponseHeader(event, 'Content-Type', CONTENT_TYPES[format]);
    setResponseHeader(event, 'Content-Disposition', `attachment; filename="${id}-cantus-firmus-${layout}.${format}"`);

    const toolkit = await createChoraleToolkit(prepared, { filters });
    const mei = exportChoraleMei(toolkit);
    toolkit.destroy();
    return mei;
});
