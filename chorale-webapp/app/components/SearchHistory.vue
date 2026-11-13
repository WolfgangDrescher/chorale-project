<script setup>
const emit = defineEmits(['apply']);

const { t, locale } = useI18n();
const { entries, forget, restore, clear } = useSearchHistory();
const toast = useToast();

const open = ref(false);
const confirmingClear = ref(false);
const selectedId = ref(null);
const selected = computed(() => entries.value.find((entry) => entry.id === selectedId.value) ?? entries.value[0]);

watch(open, (isOpen) => {
    if (!isOpen) return;
    selectedId.value = entries.value[0]?.id ?? null;
    confirmingClear.value = false;
});

function parsed(query) {
    try {
        return JSON.parse(query);
    } catch {
        return undefined;
    }
}

function summaryOf(query) {
    const value = parsed(query);
    if (Array.isArray(value) && value.length > 1) return t('searchHistoryCombinedQueries', { count: value.length });
    const single = [value].flat()[0];
    if (!single) return '';
    return [single.feature, [single.voices].flat().join(', ')].filter(Boolean).join(' · ');
}

function detailsOf(entry) {
    return [summaryOf(entry.query), t('searchHistoryMatches', { matches: entry.matches, count: entry.choraleCount }, entry.choraleCount)].filter(Boolean).join(' · ');
}

function formattedQuery(query) {
    const value = parsed(query);
    return value === undefined ? query : formatQuery(value);
}

function formatSearchedAt(timestamp) {
    return new Date(timestamp).toLocaleString(locale.value, { dateStyle: 'medium', timeStyle: 'short' });
}

function apply(entry) {
    emit('apply', entry.query);
    open.value = false;
}

function confirmClear() {
    clear();
    confirmingClear.value = false;
}

function forgetSelected() {
    const removed = selected.value;
    forget(removed.id);
    selectedId.value = entries.value[0]?.id ?? null;
    toast.add({
        title: t('searchHistoryEntryRemoved'),
        description: `${formatSearchedAt(removed.searchedAt)} · ${detailsOf(removed)}`,
        icon: 'lucide:trash',
        actions: [{
            label: t('undo'),
            color: 'neutral',
            variant: 'outline',
            onClick: () => {
                restore(removed);
                selectedId.value = removed.id;
            },
        }],
    });
}
</script>

<template>
    <UModal v-model:open="open" :title="t('searchHistory')" :ui="{ content: 'sm:max-w-4xl' }">
        <UButton
            color="neutral"
            variant="ghost"
            size="sm"
            icon="lucide:history"
            :label="t('searchHistory')"
            :disabled="!entries.length"
        />
        <template #body>
            <div v-if="selected" class="grid gap-4 md:grid-cols-[16rem_1fr]">
                <ul class="space-y-1 max-h-48 md:max-h-[60vh] overflow-y-auto">
                    <li v-for="entry in entries" :key="entry.id">
                        <button
                            type="button"
                            class="w-full rounded-md px-2 py-1.5 text-left text-sm hover:bg-elevated"
                            :class="{ 'bg-elevated': entry.id === selected.id }"
                            @click="selectedId = entry.id"
                        >
                            <span class="block tabular-nums">{{ formatSearchedAt(entry.searchedAt) }}</span>
                            <span class="block truncate text-xs text-dimmed">
                                {{ detailsOf(entry) }}
                            </span>
                        </button>
                    </li>
                </ul>
                <div class="min-w-0 space-y-4">
                    <HighlightedQuery :query="formattedQuery(selected.query)" />
                    <div class="flex flex-wrap gap-2">
                        <UButton icon="lucide:circle-check" :label="t('searchHistoryApply')" @click="apply(selected)" />
                        <UButton color="neutral" variant="ghost" icon="lucide:trash-2" :label="t('searchHistoryForget')" class="ms-auto" @click="forgetSelected" />
                    </div>
                </div>
            </div>
            <UEmpty v-else icon="lucide:history" :title="t('searchHistoryEmpty')" />
        </template>
        <template v-if="entries.length" #footer>
            <div v-if="confirmingClear" class="flex flex-wrap items-center gap-2">
                <span class="text-sm">{{ t('searchHistoryClearConfirmation') }}</span>
                <UButton color="error" size="sm" :label="t('searchHistoryClear')" @click="confirmClear" />
                <UButton color="neutral" variant="subtle" size="sm" :label="t('cancel')" @click="confirmingClear = false" />
            </div>
            <UButton v-else color="neutral" variant="link" size="sm" :label="t('searchHistoryClear')" class="px-0" @click="confirmingClear = true" />
        </template>
    </UModal>
</template>
