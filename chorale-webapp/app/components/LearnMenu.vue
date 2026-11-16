<script setup>
defineProps({
    // Cards for /learn instead of the compact row of the main menu.
    cards: { type: Boolean, default: false },
});

const { t } = useI18n();
const localePath = useLocalePath();

const { data: counts } = await useAsyncData('learn-menu-counts', async () => {
    const [lessons, faq] = await Promise.all([
        queryCollection('lessons').where('lessonNumber', 'IS NOT NULL').count(),
        queryCollection('faq').count(),
    ]);
    return { lessons, faq };
});

const items = computed(() => [
    { title: t('lessons'), description: t('lessonsDescription'), ...LEARN_PARTS.lessons, to: localePath('/lessons'), count: counts.value?.lessons },
    { title: t('faq'), description: t('faqDescription'), ...LEARN_PARTS.faq, to: localePath('/faq'), count: counts.value?.faq },
]);
</script>

<template>
    <UContainer v-if="!cards" class="grid grid-cols-2 gap-1 py-2">
        <NuxtLink v-for="item in items" :key="item.to" :to="item.to" class="group flex items-start gap-2.5 rounded-md p-2.5 hover:bg-elevated">
            <span class="flex size-8 shrink-0 items-center justify-center rounded-md" :class="item.tile"><UIcon :name="item.icon" class="size-5" /></span>
            <span class="min-w-0">
                <span class="flex items-baseline gap-1.5 text-sm font-semibold text-highlighted group-hover:text-primary">
                    {{ item.title }}
                    <span v-if="item.count" class="text-xs font-normal tabular-nums text-dimmed">{{ item.count }}</span>
                </span>
                <span class="block text-xs text-muted text-pretty">{{ item.description }}</span>
            </span>
        </NuxtLink>
    </UContainer>
    <div v-else class="grid gap-4 sm:grid-cols-2">
        <NuxtLink v-for="item in items" :key="item.to" :to="item.to" class="group flex flex-col rounded-lg border border-default p-5 hover:bg-elevated/60">
            <span class="flex items-center justify-between">
                <span class="flex size-11 items-center justify-center rounded-lg" :class="item.tile"><UIcon :name="item.icon" class="size-6" /></span>
                <span v-if="item.count" class="text-2xl font-bold tabular-nums text-highlighted">{{ item.count }}</span>
            </span>
            <span class="mt-3 block text-lg font-semibold text-highlighted group-hover:text-primary">{{ item.title }}</span>
            <span class="mt-1 block text-muted text-pretty">{{ item.description }}</span>
        </NuxtLink>
    </div>
</template>
