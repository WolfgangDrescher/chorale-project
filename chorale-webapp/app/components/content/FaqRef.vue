<script setup>
const props = defineProps({
    id: { type: String, required: true },
});

const { t } = useI18n();
const VERDICT_TEXT = { yes: 'text-green-700', no: 'text-red-700' };
const { data: faq } = await useAsyncData(`faq-ref-${props.id}`, () => {
    return queryCollection('faq').path(`/faq/${props.id}`).select('path', 'title', 'answer', 'verdict').first();
});
</script>

<template>
    <NuxtLink v-if="faq" :to="faq.path" class="not-prose group my-6 flex items-center gap-4 rounded-lg border border-default border-l-4 border-l-amber-500 bg-elevated/40 p-4 transition hover:bg-elevated">
        <UIcon :name="LEARN_PARTS.faq.icon" class="mt-0.5 size-5 shrink-0 self-start" :class="LEARN_PARTS.faq.color" />
        <span class="min-w-0 grow">
            <span class="block font-semibold text-highlighted group-hover:text-primary">{{ faq.title }}</span>
            <span class="mt-1 block text-pretty text-default">
                <span v-if="faq.verdict" class="font-bold" :class="VERDICT_TEXT[faq.verdict]">{{ t(`faqVerdict.${faq.verdict}`) }}.</span>
                {{ faq.answer }}
            </span>
        </span>
        <UIcon name="lucide:arrow-right" class="size-5 shrink-0 text-dimmed transition group-hover:translate-x-1 group-hover:text-primary" />
    </NuxtLink>
    <UAlert v-else :title="t('referenceMissing', { id })" color="warning" variant="subtle" icon="lucide:unlink" class="my-6" />
</template>
