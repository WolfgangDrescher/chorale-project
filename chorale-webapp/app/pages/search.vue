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
    const query = ref(`{
    "feature": "deg",
    "voices": "all",
    "pattern": [
        {"deg": "3"},
        {"deg": "2"},
        {"deg": ["1", "3"]}
    ]
}`);

    const choraleEntries = computed(() => Object.entries(results.value));

    const pagedChoraleEntries = computed(() => {
        const start = (page.value - 1) * CHORALES_PER_PAGE;
        return choraleEntries.value.slice(start, start + CHORALES_PER_PAGE);
    });

    async function fetchSearchResults() {
        results.value = [];
        error.value = null;
        pending.value = true;
        page.value = 1;
        try {
            results.value = await $fetch('/api/chorale-search', {
                method: 'POST',
                body: query.value,
            });
            searchFetchCompleted.value = true;
        } catch (e) {
            error.value = e;
        } finally {
            pending.value = false;
        }
    }

    return { searchFetchCompleted, choraleEntries, pagedChoraleEntries, page, error, pending, fetchSearchResults, query };
}

const {
    searchFetchCompleted,
    query,
    fetchSearchResults,
    choraleEntries,
    pagedChoraleEntries,
    page,
    pending,
    error,
} = useChoraleSearch();

function onSubmit() {
    fetchSearchResults();
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
                    <UTextarea v-model="query" class="font-mono w-full" :rows="10" />
                </UFormField>
               <UButton type="submit" :loading="pending" :trailing="true">{{ $t('submit') }}</UButton>
           </UForm>
       </UCard>
       <template v-if="error">
            {{ error }}
       </template>
        <template v-else>
            <UEmpty
                v-if="(searchFetchCompleted && choraleEntries.length === 0 && !pending) || !searchFetchCompleted"
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
                <div class="flex items-center justify-between my-4">
                    <span class="text-muted text-sm">{{ $t('choralesFound', { count: choraleEntries.length }) }}</span>
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
                            :sections="[
                                {
                                    items: items.map(i => ({
                                        voice: i.voice,
                                        startLine: i.startLine,
                                        endLine: i.endLine,
                                    })),
                                }
                            ]"
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
