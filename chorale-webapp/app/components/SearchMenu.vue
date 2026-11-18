<script setup>
const { t } = useI18n();
const localePath = useLocalePath();

const items = computed(() => [
    { title: t('search'), description: t('searchMenuSearchDescription'), icon: 'lucide:search', tile: 'bg-primary/15 text-primary', to: localePath({ name: 'search' }) },
    { title: t('grouping'), description: t('searchMenuGroupingDescription'), icon: 'lucide:chart-bar-decreasing', tile: 'bg-violet-500/15 text-violet-600', to: localePath({ name: 'grouping' }) },
]);
</script>

<template>
    <UContainer class="grid grid-cols-3 gap-1 py-2">
        <NuxtLink v-for="item in items" :key="item.to" :to="item.to" class="group flex items-start gap-2.5 rounded-md p-2.5 hover:bg-elevated">
            <span class="flex size-8 shrink-0 items-center justify-center rounded-md" :class="item.tile"><UIcon :name="item.icon" class="size-5" /></span>
            <span class="min-w-0">
                <span class="block text-sm font-semibold text-highlighted group-hover:text-primary">{{ item.title }}</span>
                <span class="block text-xs text-muted text-pretty">{{ item.description }}</span>
            </span>
        </NuxtLink>
    </UContainer>
</template>
