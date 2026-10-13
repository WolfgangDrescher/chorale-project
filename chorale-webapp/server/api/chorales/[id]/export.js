import { queryCollection } from '@nuxt/content/server';

// Downloads a chorale: /api/chorales/:id/export?format=pdf|mei&layout=score|piano&content=cantus-firmus|complete
// and, for the MEI, &voices=4|2, for the PDF &orientation=portrait|landscape.
// See chorale-mei.js for what the options mean.
const FORMATS = ['pdf', 'mei'];
const ORIENTATIONS = ['portrait', 'landscape'];

const CONTENT_TYPES = {
    pdf: 'application/pdf',
    mei: 'application/xml',
};

function invalidQuery(name, allowed) {
    return createError({
        statusCode: 422,
        statusMessage: 'Unprocessable Content',
        message: `"${name}" must be one of: ${allowed.join(', ')}`,
    });
}

// The chorale of the content (see content.config.ts), where the title of a PDF comes from.
async function findChorale(event, id) {
    const chorale = await queryCollection(event, 'chorales').where('stem', '=', `chorales/${id}`).first();
    if (!chorale) throw createError({ statusCode: 404, statusMessage: 'Page Not Found' });
    return chorale;
}

export default defineEventHandler(async (event) => {
    const { id } = getRouterParams(event);
    const {
        format = 'pdf',
        layout = 'piano',
        content = 'cantus-firmus',
        voices = '4',
        orientation = 'portrait',
    } = getQuery(event);

    if (!FORMATS.includes(format)) throw invalidQuery('format', FORMATS);
    if (!CHORALE_EXPORT_LAYOUTS.includes(layout)) throw invalidQuery('layout', CHORALE_EXPORT_LAYOUTS);
    if (!CHORALE_EXPORT_CONTENTS.includes(content)) throw invalidQuery('content', CHORALE_EXPORT_CONTENTS);
    if (!CHORALE_EXPORT_VOICES.includes(voices)) throw invalidQuery('voices', CHORALE_EXPORT_VOICES);
    if (!ORIENTATIONS.includes(orientation)) throw invalidQuery('orientation', ORIENTATIONS);

    let kern;
    try {
        kern = await loadChoraleKern(id);
    } catch {
        throw createError({ statusCode: 404, statusMessage: 'Page Not Found' });
    }

    // The whole chorale goes out as it is. Of the cantus firmus alone, the other voices are hidden
    // in a PDF (see choraleFilters), and rests in a file for a notation program.
    const filters = choraleFilters({ format, layout, content, voices });
    const prepared = format === 'mei' && content === 'cantus-firmus' ? restOtherVoices(kern) : kern;
    const chorale = format === 'pdf' ? await findChorale(event, id) : null;

    setResponseHeader(event, 'Content-Type', CONTENT_TYPES[format]);
    setResponseHeader(event, 'Content-Disposition', `attachment; filename="${id}-${content}-${layout}-${format === 'mei' ? voices : orientation}.${format}"`);

    if (format === 'mei') {
        const toolkit = await createChoraleToolkit(prepared, { filters });
        const mei = exportChoraleMei(toolkit);
        toolkit.destroy();
        return mei;
    }

    return renderChoralePdf(prepared, {
        filters,
        orientation,
        chorale,
        t: await useTranslation(event),
        siteUrl: getRequestURL(event).origin,
    });
});
