<script setup>
const { t } = useI18n();

useHead({
    title: t('search'),
});



function useChoraleSearch() {
    const results = ref([]);
    const error = ref(null);
    const pending = ref(false);
    const query = ref(`{
    "feature": "deg",
    "voices": "all",
    "pattern": [
        {"deg": "3"},
        {"deg": "2"},
        {"deg": ["1", "3"]}
    ]
}`);

    async function fetchSearchResults() {
        results.value = [];
        error.value = null;
        pending.value = true;
        try {
            results.value = await $fetch('/api/chorale-search', {
                method: 'POST',
                body: query.value,
            });
        } catch (e) {
            error.value = e;
        } finally {
            pending.value = false;
        }
    }

    return { results, error, pending, fetchSearchResults, query };
}

const { query, fetchSearchResults, results, pending, error } = useChoraleSearch();

function onSubmit() {
    fetchSearchResults();
}

</script>

 <template>
    <UContainer>
       <Heading>{{ $t('search') }}</Heading>
       <UForm class="space-y-4 my-4" @submit="onSubmit">
            <UFormField>
                <UTextarea v-model="query" class="font-mono w-full" :rows="10" />
            </UFormField>
           <UButton type="submit" :loading="pending" :trailing="true">{{ $t('submit') }}</UButton>
       </UForm>
       <template v-if="error">
            {{ error }}
       </template>
        <div v-else class="flex flex-col gap-4">
            <UCard v-for="([choraleId, items]) in Object.entries(results)" :title="choraleId">
                <HighlightedScore
                    view-mode="horizontal"
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
    </UContainer>
</template>
