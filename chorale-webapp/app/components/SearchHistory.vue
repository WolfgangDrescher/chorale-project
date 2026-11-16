<script setup>
import { onKeyStroke } from '@vueuse/core';

const emit = defineEmits(['apply']);

const { t, locale } = useI18n();
const { entries, forget, restore, clear } = useSearchHistory();
const toast = useToast();

const open = ref(false);
const confirmingClear = ref(false);
const selectedId = ref(null);
const searchText = ref('');
const searchTerms = computed(() => searchText.value.toLowerCase().split(/\s+/).filter(Boolean));
const foundEntries = computed(() => entries.value.filter((entry) => {
    const query = entry.query.toLowerCase().replace(/\s+/g, '');
    return searchTerms.value.every((term) => query.includes(term));
}));
const selected = computed(() => foundEntries.value.find((entry) => entry.id === selectedId.value) ?? foundEntries.value[0]);
const entryList = useTemplateRef('entryList');
const searchInput = useTemplateRef('searchInput');

watch(open, (isOpen) => {
    if (!isOpen) return;
    selectedId.value = entries.value[0]?.id ?? null;
    confirmingClear.value = false;
    searchText.value = '';
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

function resetSearch() {
    searchText.value = '';
    searchInput.value?.inputRef?.focus();
}

function confirmClear() {
    clear();
    confirmingClear.value = false;
}

function forgetSelected() {
    const removed = selected.value;
    const index = foundEntries.value.indexOf(removed);
    forget(removed.id);
    selectedId.value = (foundEntries.value[index] ?? foundEntries.value.at(-1))?.id ?? null;
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

function selectNeighbour(offset) {
    const neighbour = foundEntries.value[foundEntries.value.indexOf(selected.value) + offset];
    if (neighbour) selectedId.value = neighbour.id;
}

watch(() => selected.value?.id, async () => {
    await nextTick();
    entryList.value?.querySelector('[aria-current="true"]')?.scrollIntoView({ block: 'nearest' });
});

function isOnEntry(event) {
    return Boolean(entryList.value?.contains(event.target));
}

async function focusSelectedEntry() {
    await nextTick();
    const selectedEntry = entryList.value?.querySelector('[aria-current="true"]');
    (selectedEntry ?? searchInput.value?.inputRef)?.focus();
}

// Instead of the close button, where Enter would only close the modal.
function focusSearchInputOnOpen(event) {
    event.preventDefault();
    searchInput.value?.inputRef?.focus();
}

// A focused button outside the list keeps its own Enter.
function isOnOtherButton(event) {
    return Boolean(event.target.closest?.('button')) && !isOnEntry(event);
}

function whileBrowsingEntries(handler) {
    return (event) => {
        if (!open.value || !selected.value) return;
        handler(event);
    };
}

onKeyStroke(['ArrowDown', 'ArrowUp'], whileBrowsingEntries((event) => {
    event.preventDefault();
    selectNeighbour(event.key === 'ArrowDown' ? 1 : -1);
    if (isOnEntry(event)) focusSelectedEntry();
}));

onKeyStroke('Enter', whileBrowsingEntries((event) => {
    if (isOnOtherButton(event)) return;
    event.preventDefault();
    apply(selected.value);
}));

onKeyStroke(['Delete', 'Backspace'], whileBrowsingEntries((event) => {
    if (!isOnEntry(event)) return;
    event.preventDefault();
    forgetSelected();
    focusSelectedEntry();
}));
</script>

<template>
    <UModal v-model:open="open" :title="t('searchHistory')" :content="{ onOpenAutoFocus: focusSearchInputOnOpen }" :ui="{ content: 'sm:max-w-4xl' }">
        <UButton
            color="neutral"
            variant="ghost"
            size="sm"
            icon="lucide:history"
            :label="t('searchHistory')"
            :disabled="!entries.length"
        />
        <template #body>
            <div v-if="entries.length" class="grid gap-4 md:grid-cols-[16rem_1fr]">
                <div class="space-y-2">
                    <UInput ref="searchInput" v-model="searchText" icon="lucide:search" :placeholder="t('searchHistorySearch')" class="w-full">
                        <template v-if="searchText" #trailing>
                            <UButton
                                icon="lucide:x"
                                color="neutral"
                                variant="link"
                                size="sm"
                                :aria-label="t('resetFilter')"
                                :title="t('resetFilter')"
                                @click="resetSearch"
                            />
                        </template>
                    </UInput>
                    <ul v-if="selected" ref="entryList" class="space-y-1 max-h-48 md:max-h-[60vh] overflow-y-auto">
                        <li v-for="entry in foundEntries" :key="entry.id">
                            <button
                                type="button"
                                class="w-full rounded-md px-2 py-1.5 text-left text-sm outline-none"
                                :class="entry.id === selected.id ? 'bg-primary/10' : 'hover:bg-elevated'"
                                :aria-current="entry.id === selected.id"
                                @click="selectedId = entry.id"
                                @focus="selectedId = entry.id"
                            >
                                <span class="block tabular-nums">{{ formatSearchedAt(entry.searchedAt) }}</span>
                                <span class="block truncate text-xs text-dimmed">
                                    {{ detailsOf(entry) }}
                                </span>
                            </button>
                        </li>
                    </ul>
                    <p v-else class="px-2 text-sm text-dimmed">{{ t('searchHistoryNothingFound') }}</p>
                </div>
                <div v-if="selected" class="min-w-0 space-y-4">
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
