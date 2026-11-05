<script setup>
defineProps({
    count: { type: Number, required: true },
});

const { filter } = useChoraleListFilter();
</script>

<template>
    <div>
        <div class="flex items-center gap-4 mb-2">
            <UInput v-model="filter" icon="lucide:search" :placeholder="$t('filterChorales')" class="w-full max-w-sm" />
            <div class="shrink-0 whitespace-nowrap text-sm text-gray-500">{{ $t('choraleCount', { count }, count) }}</div>
        </div>

        <ChoraleListFilterBadges class="min-h-6" />
    </div>
</template>
