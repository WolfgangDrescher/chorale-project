<script setup>
defineProps({
    // { value: "102", label: "Stellen" }
    stat: { type: Object, required: true },
    size: { type: String, default: 'md' },
});
</script>

<template>
    <span class="hidden max-w-32 shrink-0 text-right sm:block">
        <span class="block font-bold leading-tight tabular-nums text-highlighted" :class="size === 'lg' ? 'text-2xl' : 'text-xl'">{{ stat.value }}</span>
        <span class="block text-xs text-muted text-pretty">{{ stat.label }}</span>
    </span>
</template>
