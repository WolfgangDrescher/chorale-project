<script setup>
const props = defineProps({
    choraleId: { type: String, required: true },
    caption: { type: String, default: '' },
    measures: { type: String, default: '' },
});

const { localScoreUrlGenerator } = useScoreUrlGenerator();
const filters = computed(() => (props.measures ? [`myank -m ${props.measures}`] : []));
</script>

<template>
    <figure class="not-prose my-6">
        <HighlightedScore :horizontal="true" :score-url="localScoreUrlGenerator(choraleId)" :filters="filters" :verovio-options="{ scale: 40, pageMarginLeft: 42 }" />
        <figcaption v-if="caption" class="mt-2 text-center text-sm text-dimmed text-pretty">
            <NuxtLink :to="`/chorale/bach/${choraleId}`" class="hover:underline">{{ caption }}</NuxtLink>
        </figcaption>
    </figure>
</template>

<style scoped>
/* Centers a score that is narrower than the page. */
figure :deep(.overflow-x-auto > .relative) {
    min-width: 0;
    margin-inline: auto;
}
</style>
