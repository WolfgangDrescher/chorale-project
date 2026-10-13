<script setup>
const props = defineProps({
    choraleId: { type: String, required: true },
    caption: { type: String, default: '' },
    outerVoices: { type: Boolean, default: true },
    intervals: { type: Boolean, default: true },
    // Only these lines of the score ("83-86"), instead of the whole chorale.
    lines: { type: String, default: '' },
    scale: { type: Number, default: 40 },
});

// The Humdrum filters, in the order the score options apply them: the lines and the voices first.
const filters = computed(() => [
    props.lines && `myank -l ${props.lines}`,
    props.outerVoices && 'extract -f 1,$',
    props.intervals && 'fb -icatm',
].filter(Boolean));
</script>

<template>
    <figure class="not-prose my-6">
        <HighlightedScore
            :horizontal="true"
            :piece-id="choraleId"
            :filters="filters"
            :verovio-options="{
                scale,
                pageMarginLeft: 42,
            }"
        />
        <figcaption v-if="caption" class="mt-2 text-sm text-dimmed">
            <NuxtLink :to="`/chorale/${choraleId}`" class="hover:underline">{{ caption }}</NuxtLink>
        </figcaption>
    </figure>
</template>
