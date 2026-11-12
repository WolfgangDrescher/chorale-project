<script setup>
import { breakpointsTailwind, useBreakpoints, useMounted } from '@vueuse/core';

const { t } = useI18n();
const localePath = useLocalePath();

useHead({
    title: t('chorales'),
});

const UButton = resolveComponent('UButton');

const { data: allChorales } = await useAsyncData('all-chorales', () => queryCollection('chorales').order('stem', 'ASC').all());

const { filter, listFilter } = useChoraleListFilter();
const { view, visibleColumns, coloring } = useChoraleListSettings();
const page = ref(1);
// Fewer chorales on a page when each comes with its whole score.
const pageSize = computed(() => (view.value === 'score' ? 10 : 20));
// Phones have room for only one page on either side of the current one. Known only once mounted,
// so the page hydrates as the server rendered it.
const isPhone = useBreakpoints(breakpointsTailwind).smaller('sm');
const isMounted = useMounted();
const paginationSiblingCount = computed(() => (isMounted.value && isPhone.value ? 1 : 2));

// What each sortable column sorts by. The number decides between equal values.
const SORT_VALUES = {
    number: (chorale) => chorale.number ?? Infinity,
    title: (chorale) => chorale.title ?? '',
    key: (chorale) => {
        const start = startKeyOf(chorale);
        return start?.signature === undefined ? Infinity : start.signature * 2 + (isMinorKey(start.key) ? 1 : 0);
    },
    meter: (chorale) => chorale.meters?.[0] ?? '',
};

const sorting = ref([{ id: 'number', desc: false }]);

watch([filter, sorting, view], () => (page.value = 1));

function compareBy(id) {
    const valueOf = SORT_VALUES[id];
    return (a, b) => {
        const [valueA, valueB] = [valueOf(a), valueOf(b)];
        return typeof valueA === 'string' ? valueA.localeCompare(valueB, 'de') : valueA - valueB;
    };
}

const chorales = computed(() => {
    const filtered = (allChorales.value ?? []).filter((item) => matchesChoraleListFilter(item, listFilter.value));
    const { id, desc } = sorting.value[0] ?? { id: 'number', desc: false };
    const [compare, compareNumbers] = [compareBy(id), compareBy('number')];
    return filtered.sort((a, b) => (desc ? -1 : 1) * compare(a, b) || compareNumbers(a, b));
});

const pagedChorales = computed(() => chorales.value.slice((page.value - 1) * pageSize.value, page.value * pageSize.value));

const linkTo = (chorale) => localePath({ name: 'chorale-id', params: { id: chorale.choraleId } });

const VIEWS = [
    { value: 'table', icon: 'lucide:list', label: 'tableView' },
    { value: 'grid', icon: 'lucide:layout-grid', label: 'gridView' },
    { value: 'score', icon: 'lucide:list-music', label: 'scoreView' },
];

const SORT_ICONS = { asc: 'lucide:arrow-up-narrow-wide', desc: 'lucide:arrow-down-wide-narrow' };

function sortableHeader(label) {
    return ({ column }) => h(UButton, {
        color: 'neutral',
        variant: 'ghost',
        size: 'sm',
        label,
        trailingIcon: SORT_ICONS[column.getIsSorted()] ?? 'lucide:arrow-up-down',
        // Pulled out by its own padding, so the label lines up with the cells below.
        class: '-mx-1 px-1 sm:-mx-2.5 sm:px-2.5 font-semibold',
        onClick: () => column.toggleSorting(column.getIsSorted() === 'asc'),
    });
}

// The smaller the screen, the fewer columns, the ones that need the most room first gone. The
// widths of the headers lay out the table, not the chorales of a page, so sorting or paging leaves
// the columns where they are. The title takes what is left.
const columns = [
    { id: 'number', header: sortableHeader(t('number')), meta: { class: { th: 'w-14 sm:w-16' } } },
    { id: 'title', header: sortableHeader(t('title')), meta: { class: { td: 'whitespace-normal' } } },
    { id: 'key', header: sortableHeader(t('key')), meta: { class: { th: 'w-24 sm:w-44' } } },
    { id: 'meter', header: sortableHeader(t('meter')), meta: { class: { th: 'hidden sm:table-cell w-28', td: 'hidden sm:table-cell text-center' } } },
    { id: 'incipit', header: t('incipit'), meta: { class: { th: 'hidden lg:table-cell w-80', td: 'hidden lg:table-cell py-0' } } },
];

// The modulations cover the bottom of a row, which gets as much more padding as they are high,
// so the text has as much room below as above.
const tableUi = computed(() => ({
    root: 'rounded-lg border border-default',
    base: 'w-full table-fixed',
    thead: 'bg-elevated/50',
    th: 'px-2 sm:px-4',
    td: ['px-2 sm:px-4 pt-2.5', visibleColumns.value.modulations ? 'pb-3.5' : 'pb-2.5'],
    tr: 'group relative cursor-pointer',
}));

// The modulations run along the bottom of a row and have no column of their own.
const columnVisibility = computed(() => {
    const { key, meter, incipit } = visibleColumns.value;
    return { key, meter, incipit };
});

const displayMenuItems = computed(() => [
    [
        { type: 'label', label: t('choraleListShow') },
        ...['key', 'meter', 'modulations', 'incipit'].map((name) => ({
            type: 'checkbox',
            label: t(name),
            checked: visibleColumns.value[name],
            onUpdateChecked: (checked) => (visibleColumns.value[name] = checked),
            onSelect: (event) => event.preventDefault(),
        })),
    ],
    [
        {
            type: 'checkbox',
            label: t('keyColoringRelative'),
            checked: coloring.value === 'relative',
            onUpdateChecked: (checked) => (coloring.value = checked ? 'relative' : 'absolute'),
            onSelect: (event) => event.preventDefault(),
        },
    ],
]);

const sortItems = computed(() => Object.keys(SORT_VALUES).map((id) => ({ label: t(id), value: id })));
const sortId = computed({
    get: () => sorting.value[0]?.id ?? 'number',
    set: (id) => (sorting.value = [{ id, desc: false }]),
});
const toggleSortDirection = () => (sorting.value = [{ id: sortId.value, desc: !sorting.value[0]?.desc }]);
</script>

<template>
    <UContainer>
        <Heading>{{ $t('chorales') }}</Heading>

        <div v-if="!allChorales?.length">
            <UAlert
                icon="lucide:circle-alert"
                color="warning"
                variant="subtle"
                :title="$t('noChoralesFound')"
                :description="$t('noChoralesFoundDescription')"
            />
        </div>

        <template v-else>
            <div class="flex flex-wrap items-start justify-between gap-x-4 gap-y-2 mb-4">
                <ChoraleListFilter :count="chorales.length" class="grow" />
                <div class="flex items-center gap-2">
                    <USelect v-model="sortId" :items="sortItems" size="sm" class="w-28 sm:w-36" :aria-label="$t('sortBy')" />
                    <UButton
                        :icon="SORT_ICONS[sorting[0]?.desc ? 'desc' : 'asc']"
                        color="neutral"
                        variant="outline"
                        size="sm"
                        :aria-label="$t('sortDirection')"
                        @click="toggleSortDirection"
                    />
                    <!-- The room of a check mark stays when it is unchecked, so the menu keeps its width. -->
                    <UDropdownMenu :items="displayMenuItems" :content="{ align: 'end' }" :ui="{ itemTrailing: 'min-w-5' }">
                        <UButton icon="lucide:settings-2" color="neutral" variant="outline" size="sm" :aria-label="$t('choraleListShow')" />
                    </UDropdownMenu>
                    <UFieldGroup size="sm">
                        <UButton
                            v-for="{ value, icon, label } in VIEWS"
                            :key="value"
                            :icon="icon"
                            color="neutral"
                            :variant="view === value ? 'subtle' : 'outline'"
                            :aria-label="$t(label)"
                            :aria-pressed="view === value"
                            @click="view = value"
                        />
                    </UFieldGroup>
                </div>
            </div>

            <UTable
                v-if="view === 'table'"
                v-model:sorting="sorting"
                :column-visibility="columnVisibility"
                :sorting-options="{ manualSorting: true }"
                :get-row-id="(chorale) => chorale.choraleId"
                :data="pagedChorales"
                :columns="columns"
                :ui="tableUi"
                @select="(_, row) => navigateTo(linkTo(row.original))"
            >
                <template #number-cell="{ row }">
                    <span class="text-2xl font-light tabular-nums text-dimmed group-hover:text-primary">{{ row.original.number }}</span>
                </template>
                <template #title-cell="{ row }">
                    <NuxtLink :to="linkTo(row.original)" class="block font-medium text-highlighted group-hover:text-primary">
                        {{ row.original.title }}
                    </NuxtLink>
                    <ChoraleListItemDetails :chorale="row.original" />
                    <!-- Along the bottom of the whole row, which is its positioned ancestor. -->
                    <ChoraleModulationMap
                        v-if="visibleColumns.modulations && row.original.modulations && row.original.duration"
                        :modulations="row.original.modulations"
                        :duration="row.original.duration"
                        :relative="coloring === 'relative'"
                        class="absolute inset-x-0 bottom-0 h-1"
                    />
                </template>
                <template #key-cell="{ row }">
                    <ChoraleKeyBadge
                        v-if="startKeyOf(row.original)"
                        :key-context="startKeyOf(row.original)"
                        :cantus-firmus="differingCantusFirmusKeyOf(row.original)"
                    />
                </template>
                <template #meter-cell="{ row }">
                    <ChoraleMeter v-if="row.original.meters" :meters="row.original.meters" class="text-toned" />
                </template>
                <template #incipit-cell="{ row }">
                    <ChoraleIncipit :chorale-id="row.original.choraleId" class="h-14 w-72" />
                </template>
            </UTable>

            <div v-else-if="view === 'score'" class="flex flex-col gap-4">
                <ChoraleScoreItem
                    v-for="chorale in pagedChorales"
                    :key="chorale.choraleId"
                    :chorale="chorale"
                    :to="linkTo(chorale)"
                    :visible-columns="visibleColumns"
                    :relative-coloring="coloring === 'relative'"
                />
            </div>

            <div v-else class="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 xl:grid-cols-4 gap-3">
                <ChoraleGridItem
                    v-for="chorale in pagedChorales"
                    :key="chorale.choraleId"
                    :chorale="chorale"
                    :to="linkTo(chorale)"
                    :visible-columns="visibleColumns"
                    :relative-coloring="coloring === 'relative'"
                />
            </div>

            <div class="flex justify-center mt-4">
                <UPagination v-model:page="page" :items-per-page="pageSize" :total="chorales.length" :sibling-count="paginationSiblingCount" />
            </div>
        </template>
    </UContainer>
</template>
