// Each chorale by its id, from the metadata extract-chorales.mjs wrote.
export function useChoraleMetadata() {
    const { data: chorales } = useAsyncData('chorales-by-id', async () => {
        const entries = await queryCollection('chorales').select('choraleId', 'title', 'bwv', 'duration').all();
        return Object.fromEntries(entries.map((chorale) => [chorale.choraleId, chorale]));
    });
    return chorales;
}
