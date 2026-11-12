<script setup>
// The meters of a chorale as time signatures side by side: the numbers stacked, without a line
// between them.
defineProps({
    meters: { type: Array, required: true },
});
</script>

<template>
    <span class="inline-flex items-center gap-1.5 text-sm font-extrabold leading-[0.85] tabular-nums">
        <span v-for="meter in meters" :key="meter" class="inline-flex flex-col items-center" :aria-label="meter">
            <span v-for="(part, index) in meter.split('/')" :key="index" aria-hidden="true">{{ part }}</span>
        </span>
    </span>
</template>
