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

    const choraleEntries = computed(() => Object.entries(results.value));

    const totalMatches = computed(() => choraleEntries.value.reduce((sum, [, items]) => sum + items.length, 0));

    const pagedChoraleEntries = computed(() => {
        const start = (page.value - 1) * CHORALES_PER_PAGE;
        return choraleEntries.value.slice(start, start + CHORALES_PER_PAGE);
    });

    async function fetchSearchResults() {
        results.value = [];
        error.value = null;
        pending.value = true;
        page.value = 1;
        durationMs.value = null;
        try {
            // durationMs is the server's measurement of the chorale-search binary
            // alone, so the number stays independent of network and render time.
            const response = await $fetch('/api/chorale-search', {
                method: 'POST',
                body: query.value,
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

    return { searchFetchCompleted, choraleEntries, totalMatches, pagedChoraleEntries, page, error, pending, durationMs, fetchSearchResults, query };
}

const {
    searchFetchCompleted,
    query,
    fetchSearchResults,
    choraleEntries,
    totalMatches,
    pagedChoraleEntries,
    page,
    pending,
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
            <div v-if="pending" class="flex flex-col gap-6 mt-8">
                <div  v-for="n in 3" class="grid gap-2 mx-auto">
                    <USkeleton class="h-4 w-[250px]" />
                    <USkeleton class="h-4 w-[200px]" />
                </div>
            </div>
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
                            <span v-if="durationMs !== null" class="text-dimmed tabular-nums">({{ $t('searchDuration', { ms: durationMs }) }})</span>
                        </template>
                    </i18n-t>
                    <UPagination v-model:page="page" :total="choraleEntries.length" :items-per-page="CHORALES_PER_PAGE" size="xs" />
                </div>
                <div class="flex flex-col gap-4">
                    <UCard v-for="([choraleId, items]) in pagedChoraleEntries" :key="choraleId" :title="choraleId">
                        <HighlightedScore
                            :horizontal="true"
                            :piece-id="choraleId"
                            :verovio-options="{
                                scale: 35,
                                pageMarginLeft: 42,
                            }"
                            :sections="sectionsForItems(items)"
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
