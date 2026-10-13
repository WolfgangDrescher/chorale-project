<script setup>
// Opening the drawer starts from the given query again, and searches unless the results shown are
// already those of that query.
const props = defineProps({
    query: { type: String, required: true },
    sortBy: { type: String, default: 'id' },
});

const open = defineModel('open', { type: Boolean, default: false });

const { t } = useI18n();
const search = useChoraleSearch(props.query, { sortBy: props.sortBy });

watch(open, (isOpen) => {
    if (!isOpen || search.pending.value) return;
    search.query.value = props.query;
    if (search.searchedQuery.value !== search.query.value) {
        search.fetchSearchResults();
    }
});
</script>

<template>
    <UDrawer
        v-model:open="open"
        direction="right"
        :handle="false"
        handle-only
        close
        :title="t('searchResults')"
        :ui="{ content: 'w-full max-w-4xl select-text!', container: 'min-w-0 overflow-x-hidden' }"
    >
        <template #body>
            <UCollapsible class="mb-4">
                <UButton
                    color="neutral"
                    variant="link"
                    size="xs"
                    trailing-icon="lucide:chevron-down"
                    :label="t('editQuery')"
                    class="px-0"
                />
                <template #content>
                    <UForm class="mt-2 space-y-4" @submit="search.fetchSearchResults">
                        <MonacoEditor
                            v-model="search.query.value"
                            :schema="choraleSearchQuerySchema"
                            :min-height="100"
                            :max-height="320"
                            :options="{
                                fontSize: 14,
                                tabSize: 12,
                                scrollBeyondLastLine: false,
                                automaticLayout: true,
                                scrollbar: {
                                    alwaysConsumeMouseWheel: false,
                                },
                            }"
                        />
                        <UButton v-if="search.pending.value || search.searchedQuery.value !== search.query.value" type="submit" :loading="search.pending.value">{{ t('submit') }}</UButton>
                    </UForm>
                </template>
            </UCollapsible>
            <SearchResults :search="search" />
        </template>
    </UDrawer>
</template>
