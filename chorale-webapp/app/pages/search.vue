<script setup>
const { t } = useI18n();

const localePath = useLocalePath();

useHead({
    title: t('search'),
});

const CHORALES_PER_PAGE = 10;

function useChoraleSearch() {
    const searchFetchCompleted = ref(false);
    const results = ref([]);
    const error = ref(null);
    const pending = ref(false);
    const progress = ref(null);
    const page = ref(1);
    const durationMs = ref(null);
    const query = ref(`{
    "feature": "deg",
    "voices": "all",
    "pattern": [
        { "deg": "3" },
        { "deg": "2" },
        { "deg": ["1", "3"] }
    ]
}`);

    // The order of the chorales: by their id (ascending is the natural one) or by how many matches
    // each has (most first). Chorales with as many matches stay in the order of their ids.
    const sortBy = ref('id');
    const sortDescending = ref(false);

    watch(sortBy, (value) => {
        sortDescending.value = value === 'matches';
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
        return entries.sort((a, b) => direction * (sortBy.value === 'matches' ? byMatches(a, b) || byId(a, b) : byId(a, b)));
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
    };
}

const {
    searchFetchCompleted,
    query,
    fetchSearchResults,
    choraleEntries,
    totalMatches,
    pagedChoraleEntries,
    page,
    sortBy,
    sortDescending,
    pending,
    progress,
    durationMs,
    error,
} = useChoraleSearch();

function onSubmit() {
    fetchSearchResults();
}

// Splits a chorale's results into one HighlightedScore section group per
// queryId, so results from different combined queries get visually distinct
// colors. Results without a queryId (a single, non-array query) all fall into
// one group with no explicit color, picking up HighlightedScore's own default.
function colorForQueryId(queryId) {
    if (queryId === '') return undefined;
    if (queryId in highlightColorsByName) return highlightColorsByName[queryId]; // e.g. a query id of "green" forces that exact color
    const colorIndex = Number(queryId);
    return Number.isInteger(colorIndex) ? defaultHighlightColors[colorIndex % defaultHighlightColors.length] : undefined;
}

function sectionsForItems(items) {
    const itemsByQueryId = new Map();
    for (const item of items) {
        const queryId = item.queryId ?? '';
        if (!itemsByQueryId.has(queryId)) itemsByQueryId.set(queryId, []);
        itemsByQueryId.get(queryId).push({
            voice: item.voice,
            startLine: item.startLine,
            endLine: item.endLine,
        });
    }

    return Array.from(itemsByQueryId.entries()).map(([queryId, sectionItems]) => ({
        items: sectionItems,
        color: colorForQueryId(queryId),
    }));
}

function applyDemoQuery() {
    query.value = `{
    "feature": "kern",
    "voices": "1234",
    "pattern": [
        { "deg": "3", "duration": "4" },
        { "deg": "2", "duration": "4" },
        { "deg": ["1", "3"], "duration": "*", "fermata": true }
    ],
    "mintStartAtPreviousToken": true,
    "fbCompareExactChord": false,
    "limit": 100
}`;
}
</script>

 <template>
    <UContainer>
       <Heading>{{ $t('search') }}</Heading>

       <UCard class="mb-4">
           <UForm class="space-y-4" @submit="onSubmit">
                <UFormField :label="$t('query')">
                    <MonacoEditor
                        v-model="query"
                        :schema="choraleSearchQuerySchema"
                        :options="{
                            fontSize: 14,
                            // theme: 'vs-light',
                            tabSize: 12,
                            scrollBeyondLastLine: false,
                            automaticLayout: true,
                            scrollbar: {
                                alwaysConsumeMouseWheel: false,
                            },
                        }"
                    />
                </UFormField>
               <UButton type="submit" :loading="pending" :trailing="true">{{ $t('submit') }}</UButton>
           </UForm>
       </UCard>
       <template v-if="error">
            <UAlert color="error" variant="subtle" :title="error.data?.message ?? $t('searchError')">
                <template v-if="error.data?.errors?.length" #description>
                    <ul>
                        <li v-for="(msg, i) in error.data.errors" :key="i">{{ msg }}</li>
                    </ul>
                </template>
            </UAlert>
       </template>
        <template v-else>
            <SearchProgress v-if="pending" :progress="progress" class="mt-8" />
            <UEmpty
                v-else-if="(searchFetchCompleted && choraleEntries.length === 0) || !searchFetchCompleted"
                :title="searchFetchCompleted ? $t('noResults') : undefined"
                :description="$t('noResultsDescription')"
                icon="lucide:file-search"
                class="md:w-1/2 lg:w-1/3 mx-auto"
                :actions="[
                    {
                        icon: 'lucide:file-text',
                        label: $t('readDocs'),
                        to: localePath('/docs'),
                    },
                    {
                        icon: 'lucide:settings',
                        label: $t('applyDemoQuery'),
                        onClick: applyDemoQuery,
                        color: 'neutral',
                        variant: 'subtle',
                    },
                    {
                        icon: 'lucide:flask-conical',
                        label: $t('exampleQueries'),
                        to: localePath('/docs/examples'),
                        color: 'neutral',
                        variant: 'subtle',
                    }
                ]"
            />
            <template v-else>
                <div class="flex items-center justify-between gap-4 my-4">
                    <i18n-t keypath="matchesFound" :plural="choraleEntries.length" tag="p" class="text-sm" scope="global">
                        <template #matches>{{ totalMatches }}</template>
                        <template #duration>
                            <span v-if="durationMs !== null" class="text-dimmed tabular-nums">({{ $t('searchDuration', { duration: formatDuration(durationMs) }) }})</span>
                        </template>
                    </i18n-t>
                    <div class="flex items-center gap-2">
                        <UFieldGroup>
                            <USelect
                                v-model="sortBy"
                                :items="[
                                    { label: $t('sortById'), value: 'id' },
                                    { label: $t('sortByMatches'), value: 'matches' },
                                ]"
                                size="xs"
                                class="w-28"
                                :aria-label="$t('sortBy')"
                            />
                            <UButton
                                color="neutral"
                                variant="outline"
                                size="xs"
                                :icon="sortDescending ? 'lucide:arrow-down-wide-narrow' : 'lucide:arrow-up-narrow-wide'"
                                :aria-label="sortDescending ? $t('sortDescending') : $t('sortAscending')"
                                @click="sortDescending = !sortDescending"
                            />
                        </UFieldGroup>
                        <UPagination v-model:page="page" :total="choraleEntries.length" :items-per-page="CHORALES_PER_PAGE" size="xs" />
                    </div>
                </div>
                <div class="flex flex-col gap-4">
                    <UCard v-for="([choraleId, items]) in pagedChoraleEntries" :key="choraleId">
                        <template #header>
                            <div class="flex items-center justify-between gap-4">
                                <span>{{ choraleId }}</span>
                                <UBadge color="neutral" variant="subtle" :label="$t('matchCount', items.length)" />
                            </div>
                        </template>
                        <HighlightedScore
                            :horizontal="true"
                            :piece-id="choraleId"
                            :verovio-options="{
                                scale: 35,
                                pageMarginLeft: 42,
                            }"
                            :sections="sectionsForItems(items)"
                            :scroll-to-first-section="true"
                        />
                    </UCard>
                </div>
                <div class="flex justify-center my-4">
                    <UPagination v-model:page="page" :total="choraleEntries.length" :items-per-page="CHORALES_PER_PAGE" />
                </div>
            </template>
        </template>
    </UContainer>
</template>
