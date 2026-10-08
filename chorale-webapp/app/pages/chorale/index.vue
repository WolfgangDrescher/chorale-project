<script setup>
const { t } = useI18n();
const localePath = useLocalePath();

useHead({
    title: t('chorales'),
});

const { data: allChorales } = await useAsyncData('all-chorales', () => queryCollection('chorales').order('stem', 'ASC').all());

const filter = ref('');
const page = ref(1);
const PAGE_SIZE = 20;

watch(filter, () => (page.value = 1));

const chorales = computed(() => {
    const needle = filter.value.trim().toLowerCase();
    return (allChorales.value ?? [])
        .map((item) => ({
            choraleId: item.choraleId,
            number: item.number,
            bwv: item.bwv,
            title: item.title,
            key: [item.key, item.mode].filter(Boolean).join(' '),
            meter: item.meter,
        }))
        .filter((item) => !needle || [item.choraleId, item.bwv, item.title, item.key, item.meter]
            .some((value) => value?.toLowerCase().includes(needle)));
});

const pagedChorales = computed(() => chorales.value.slice((page.value - 1) * PAGE_SIZE, page.value * PAGE_SIZE));

const columns = [
    { accessorKey: 'choraleId', header: t('id') },
    { accessorKey: 'bwv', header: t('bwv') },
    { accessorKey: 'title', header: t('title') },
    { accessorKey: 'key', header: t('key') },
    { accessorKey: 'meter', header: t('meter') },
];
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
            <div class="flex items-center gap-4 mb-4">
                <UInput v-model="filter" icon="lucide:search" :placeholder="$t('filterChorales')" class="w-full max-w-sm" />
                <div class="text-sm text-gray-500">{{ $t('choraleCount', { count: chorales.length }, chorales.length) }}</div>
            </div>

            <UTable :data="pagedChorales" :columns="columns">
                <template #choraleId-cell="{ row }">
                    <ULink :to="localePath({ name: 'chorale-id', params: { id: row.original.choraleId } })" class="font-mono text-primary">
                        {{ row.original.choraleId }}
                    </ULink>
                </template>
                <template #title-cell="{ row }">
                    <ULink :to="localePath({ name: 'chorale-id', params: { id: row.original.choraleId } })">
                        {{ row.original.title }}
                    </ULink>
                </template>
            </UTable>

            <div class="flex justify-center mt-4">
                <UPagination v-model:page="page" :items-per-page="PAGE_SIZE" :total="chorales.length" />
            </div>
        </template>
    </UContainer>
</template>
