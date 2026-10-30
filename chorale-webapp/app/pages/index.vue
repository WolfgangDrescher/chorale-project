<script setup>
const { t } = useI18n();
const localePath = useLocalePath();

useHead({
    title: t('home'),
});

const links = [
    { label: t('chorales'), icon: 'lucide:music', to: localePath({ name: 'chorale' }) },
    { label: t('search'), icon: 'lucide:search', to: localePath({ name: 'search' }) },
    { label: t('lessons'), icon: 'lucide:graduation-cap', to: localePath('/lessons') },
    { label: t('docs'), icon: 'lucide:book-open', to: localePath('/docs') },
];
</script>

<template>
    <UContainer>
        <div class="flex flex-col md:flex-row items-center gap-8 md:gap-12 my-8">
            <img src="/favicon.svg" alt="" class="w-40 h-40 md:w-56 md:h-56 shrink-0" />
            <div class="text-center md:text-left">
                <Heading>Chorale Project</Heading>
                <div class="flex flex-col gap-6">
                    <p class="text-lg text-pretty">{{ t('intro') }}</p>
                    <div class="flex flex-wrap justify-center md:justify-start gap-2">
                        <UButton v-for="link in links" :key="link.to" v-bind="link" color="neutral" variant="outline" />
                    </div>
                </div>
            </div>
        </div>
        <UAlert :title="t('workInProgress')" :description="t('workInProgressDescription')" icon="lucide:construction" color="neutral" variant="subtle" class="my-8" />
    </UContainer>
</template>
