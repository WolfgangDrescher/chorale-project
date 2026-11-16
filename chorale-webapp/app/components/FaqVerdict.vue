<script setup>
defineProps({
    verdict: { type: String, default: null },
});

const { t } = useI18n();
const CLASSES = { yes: 'bg-green-500/15 text-green-700', no: 'bg-red-500/15 text-red-700' };
</script>

<template>
    <span class="flex h-7 w-12 shrink-0 items-center justify-center rounded-md text-sm font-bold" :class="CLASSES[verdict] ?? 'bg-amber-500/15 text-amber-700'">
        <template v-if="verdict">{{ t(`faqVerdict.${verdict}`) }}</template>
        <UIcon v-else :name="LEARN_PARTS.faq.icon" class="size-4" />
    </span>
</template>
