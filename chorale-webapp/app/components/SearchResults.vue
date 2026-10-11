<script setup>
import { useMediaQuery } from '@vueuse/core';

// The outcome of a search made with useChoraleSearch: the error or progress while it runs, and
// afterwards the matches per chorale. The empty slot replaces what is shown before the first
// search and when there are no matches.
const props = defineProps({
    search: { type: Object, required: true },
});

const {
    searchFetchCompleted,
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
} = props.search;

// Few page buttons and no jump to the first and last page on a narrow screen, where the
// pagination would otherwise be wider than the screen.
const isWideScreen = useMediaQuery('(min-width: 640px)');
const paginationProps = computed(() => ({
    siblingCount: isWideScreen.value ? 2 : 1,
    ui: { first: 'hidden sm:inline-flex', last: 'hidden sm:inline-flex' },
}));

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
</script>

<template>
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
        <slot
            v-else-if="(searchFetchCompleted && choraleEntries.length === 0) || !searchFetchCompleted"
            name="empty"
            :completed="searchFetchCompleted"
        >
            <UEmpty
                :title="searchFetchCompleted ? $t('noResults') : undefined"
                :description="searchFetchCompleted ? undefined : $t('noResultsDescription')"
                icon="lucide:file-search"
            />
        </slot>
        <template v-else>
            <div class="flex flex-wrap items-center justify-between gap-4 my-4">
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
                    <UPagination v-model:page="page" :total="choraleEntries.length" :items-per-page="CHORALES_PER_PAGE" size="xs" v-bind="paginationProps" />
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
                <UPagination v-model:page="page" :total="choraleEntries.length" :items-per-page="CHORALES_PER_PAGE" v-bind="paginationProps" />
            </div>
        </template>
    </template>
</template>
