// Saves { modulations: [["0/4", "G"], ...] } of a chorale and generates it anew; answers with the
// warnings and the modulations of the new score.
export default defineEventHandler(async (event) => {
    if (!import.meta.dev) throw createError({ statusCode: 404, statusMessage: 'Page Not Found' });

    const { id } = getRouterParams(event);
    if (!/^chor\d{3}$/.test(id)) throw createError({ statusCode: 404, statusMessage: 'Page Not Found' });

    const { modulations } = await readBody(event) ?? {};
    const isEntry = (entry) => Array.isArray(entry) && entry.length === 2 && /^\d+\/\d+(\+\d+\/\d+)?$/.test(entry[0]) && /^[A-Ga-g][#-]*(:(ion|dor|phr|lyd|mix|aeo|loc))?$/.test(entry[1]);
    if (!Array.isArray(modulations) || !modulations.every(isEntry)) {
        throw createError({ statusCode: 422, statusMessage: 'Unprocessable Content', message: '"modulations" must be a list of ["measure/beat", "key"] pairs' });
    }

    await writeChoraleModulations(id, modulations);
    try {
        const warnings = await regenerateChorale(id);
        const { modulations: generated } = await readPositions(id);
        return { warnings, modulations: generated };
    } catch (error) {
        throw createError({ statusCode: 500, statusMessage: 'Internal Server Error', message: error.message });
    }
});
