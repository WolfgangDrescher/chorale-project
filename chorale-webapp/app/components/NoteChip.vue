<script setup>
// A chip such as the ones on the circles of notes: an icon or a single character in a circle, a longer text
// in a pill it fits in.
const props = defineProps({
    icon: { type: String, default: undefined },
    text: { type: String, default: undefined },
    color: { type: String, required: true },
    // In whole pixels: a fractional size rounds the circle to an ellipse.
    size: { type: Number, required: true },
});

// Even, so the icon has as much room on each side.
const evenPixels = (size) => Math.max(2, Math.round(size / 2) * 2);
const inCircle = computed(() => !!props.icon || props.text?.length === 1);

// Lucide draws with lines of 2 in 24, too thin for a chip of a few pixels.
const withThickerLines = (svg) => svg.replaceAll('stroke-width="2"', 'stroke-width="3"');
</script>

<template>
    <span
        class="inline-flex shrink-0 items-center justify-center whitespace-nowrap rounded-full font-semibold leading-none text-white"
        :style="{
            height: `${size}px`,
            ...(inCircle ? { width: `${size}px` } : { padding: `0 ${size / 3}px` }),
            fontSize: `${evenPixels(size * 0.8)}px`,
            background: color,
        }"
    >
        <UIcon v-if="icon" :name="icon" mode="svg" :customize="withThickerLines" :style="{ width: `${evenPixels(size * 0.75)}px`, height: `${evenPixels(size * 0.75)}px` }" />
        <template v-else>{{ text }}</template>
    </span>
</template>
