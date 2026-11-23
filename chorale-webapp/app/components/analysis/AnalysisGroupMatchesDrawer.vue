<script setup>
// The matches of a group that no query finds: its group query is run again with ?matches=true, which
// lists them, and the group is found in its result by its values at each level.
const props = defineProps({
    groupQuery: { type: Object, required: true },
    variant: { type: String, default: '' },
    path: { type: Array, required: true },
    member: { type: Array, default: null },
});

const open = defineModel('open', { type: Boolean, default: false });

const { t } = useI18n();
const search = useChoraleSearch('', { sortBy: 'density' });

const sameValues = (a, b) => JSON.stringify(a) === JSON.stringify(b);

function matchesIn(result) {
    let group = props.variant ? result.variants?.[props.variant] : result;
    for (const values of props.path) group = group?.groups?.find((below) => sameValues(below.values, values));
    if (props.member) group = group?.members?.find((member) => sameValues(member.values, props.member));
    return group?.matches ?? {};
}

watch(open, async (isOpen) => {
    if (!isOpen) return;
    search.results.value = [];
    search.error.value = null;
    search.pending.value = true;
    try {
        const response = await fetchWithProgress('/api/chorale-group?matches=true', { body: props.groupQuery });
        search.results.value = matchesIn(response.result);
        search.durationMs.value = response.durationMs;
        search.searchFetchCompleted.value = true;
    } catch (e) {
        search.error.value = e;
    } finally {
        search.pending.value = false;
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
            <SearchResults :search="search" />
        </template>
    </UDrawer>
</template>
