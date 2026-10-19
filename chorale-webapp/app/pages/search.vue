<script setup>
import { useLocalStorage } from '@vueuse/core';

const { t } = useI18n();

const localePath = useLocalePath();

useHead({
    title: t('search'),
});

// The query stays in local storage, so it is still there when the page is visited again.
const storedQuery = useLocalStorage('chorale-search-query', `{
    "feature": "deg",
    "voices": "all",
    "pattern": [
        { "deg": "3" },
        { "deg": "2" },
        { "deg": ["1", "3"] }
    ]
}`);

const search = useChoraleSearch(storedQuery);

const { query, pending, fetchSearchResults } = search;

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
       <SearchResults :search="search">
            <template #empty="{ completed }">
                <UEmpty
                    :title="completed ? $t('noResults') : undefined"
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
            </template>
        </SearchResults>
    </UContainer>
</template>
