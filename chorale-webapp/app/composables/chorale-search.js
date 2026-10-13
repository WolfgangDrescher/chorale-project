export const CHORALES_PER_PAGE = 10;

// The state of one search: the query, its results (grouped by chorale), how they are sorted and
// paged, and the progress of a running search.
export function useChoraleSearch(initialQuery = '') {
    const searchFetchCompleted = ref(false);
    const results = ref([]);
    const error = ref(null);
    const pending = ref(false);
    const progress = ref(null);
    const page = ref(1);
    const durationMs = ref(null);
    const query = ref(initialQuery);
    // The query the current results are for, once a search has succeeded.
    const searchedQuery = ref(null);

    // The length of each chorale in quarter notes, from the metadata extract-chorales.mjs wrote.
    const { data: durations } = useAsyncData('chorale-durations', async () => {
        const chorales = await queryCollection('chorales').select('choraleId', 'duration').all();
        return Object.fromEntries(chorales.map(({ choraleId, duration }) => [choraleId, duration]));
    });

    // The order of the chorales: by their id (ascending is the natural one), by how many matches
    // each has (most first), or by how many matches fall on each quarter note of it (most first), so
    // a long chorale does not win just by having more room. Chorales that tie stay in the order of their ids.
    const sortBy = ref('id');
    const sortDescending = ref(false);

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
        const density = ([choraleId, items]) => (durations.value?.[choraleId] ? items.length / durations.value[choraleId] : 0);
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
