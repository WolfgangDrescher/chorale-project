export const CHORALES_PER_PAGE = 10;

// The state of one search: the query, its results (grouped by chorale), how they are sorted and
// paged, and the progress of a running search. The initial query can be a ref, e.g. one kept in
// local storage, which then holds the query of the search.
export function useChoraleSearch(initialQuery = '', { sortBy: initialSortBy = 'id' } = {}) {
    const searchFetchCompleted = ref(false);
    const results = ref([]);
    const error = ref(null);
    const pending = ref(false);
    const progress = ref(null);
    const page = ref(1);
    const durationMs = ref(null);
    const query = toRef(initialQuery);
    // The query the current results are for, once a search has succeeded.
    const searchedQuery = ref(null);

    // Each chorale by its id: its title and BWV number for the results, and its length in quarter
    // notes for their density, from the metadata extract-chorales.mjs wrote.
    const { data: chorales } = useAsyncData('chorales-by-id', async () => {
        const entries = await queryCollection('chorales').select('choraleId', 'title', 'bwv', 'duration').all();
        return Object.fromEntries(entries.map((chorale) => [chorale.choraleId, chorale]));
    });

    // The order of the chorales: by their id (ascending is the natural one), by how many matches
    // each has (most first), or by how many matches fall on each quarter note of it (most first), so
    // a long chorale does not win just by having more room. Chorales that tie stay in the order of their ids.
    const sortBy = ref(initialSortBy);
    const sortDescending = ref(initialSortBy !== 'id');

    watch(sortBy, (value) => {
        sortDescending.value = value !== 'id';
        page.value = 1;
    });
    watch(sortDescending, () => {
        page.value = 1;
    });

    const choraleEntries = computed(() => {
        const entries = Object.entries(results.value);
        const direction = sortDescending.value ? -1 : 1;
        const byId = ([a], [b]) => a.localeCompare(b, undefined, { numeric: true });
        const byMatches = ([, a], [, b]) => a.length - b.length;
        const duration = (choraleId) => chorales.value?.[choraleId]?.duration;
        const density = ([choraleId, items]) => (duration(choraleId) ? items.length / duration(choraleId) : 0);
        const byDensity = (a, b) => density(a) - density(b);
        const comparators = { id: byId, matches: byMatches, density: byDensity };
        const compare = comparators[sortBy.value];
        return entries.sort((a, b) => direction * (sortBy.value === 'id' ? byId(a, b) : compare(a, b) || byId(a, b)));
    });

    const totalMatches = computed(() => choraleEntries.value.reduce((sum, [, items]) => sum + items.length, 0));

    const pagedChoraleEntries = computed(() => {
        const start = (page.value - 1) * CHORALES_PER_PAGE;
        return choraleEntries.value.slice(start, start + CHORALES_PER_PAGE);
    });

    async function fetchSearchResults() {
        results.value = [];
        error.value = null;
        pending.value = true;
        progress.value = null;
        page.value = 1;
        durationMs.value = null;
        searchedQuery.value = null;
        const searching = query.value;
        try {
            // durationMs is the server's measurement of the chorale-search binary
            // alone, so the number stays independent of network and render time.
            const response = await fetchWithProgress('/api/chorale-search', {
                body: query.value,
                onProgress: (event) => {
                    progress.value = { ...progress.value, ...event };
                },
            });
            results.value = response.results;
            durationMs.value = response.durationMs;
            searchedQuery.value = searching;
            searchFetchCompleted.value = true;
        } catch (e) {
            error.value = e;
        } finally {
            pending.value = false;
        }
    }

    return {
        searchFetchCompleted,
        chorales,
        choraleEntries,
        totalMatches,
        pagedChoraleEntries,
        page,
        sortBy,
        sortDescending,
        error,
        pending,
        progress,
        durationMs,
        fetchSearchResults,
        query,
        searchedQuery,
    };
}
