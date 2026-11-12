<script setup>
// A meter as a time signature: the numbers stacked, without a line between them.
const props = defineProps({
    meter: { type: String, required: true },
});

const parts = computed(() => props.meter.split('/'));
</script>

<template>
    <span class="inline-flex flex-col items-center text-sm font-extrabold leading-[0.85] tabular-nums" :aria-label="meter">
        <span v-for="(part, index) in parts" :key="index" aria-hidden="true">{{ part }}</span>
    </span>
</template>
