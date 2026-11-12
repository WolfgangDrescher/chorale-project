<script setup>
// The keys of a chorale as a bar, each as wide as it lasts.
const props = defineProps({
    // { key, mode, signature, durationFromStart }, the starting key first.
    modulations: { type: Array, required: true },
    // In quarter notes, as durationFromStart.
    duration: { type: Number, required: true },
    relative: { type: Boolean, default: false },
});

const { locale } = useI18n();

const homeSignature = computed(() => (props.relative ? (props.modulations[0]?.signature ?? 0) : 0));

const segments = computed(() => props.modulations.map((modulation, index) => ({
    title: modulation.key === '?' ? '?' : localizedKeyName(modulation.key, locale.value),
    appearance: keyAppearance(modulation, homeSignature.value),
    width: ((props.modulations[index + 1]?.durationFromStart ?? props.duration) - modulation.durationFromStart) / props.duration,
})));
</script>

<template>
    <span class="flex gap-px overflow-hidden">
        <span
            v-for="(segment, index) in segments"
            :key="index"
            class="h-full"
            :class="segment.appearance.class"
            :style="{ ...segment.appearance.style, width: `${segment.width * 100}%` }"
            :title="segment.title"
        />
    </span>
</template>
