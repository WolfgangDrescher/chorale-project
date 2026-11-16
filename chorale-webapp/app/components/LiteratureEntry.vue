<script setup>
const props = defineProps({
    entry: { type: Object, required: true },
});

const { d } = useI18n();
const formatDate = (date) => d(new Date(date), { dateStyle: 'medium', timeZone: 'UTC' });
const segments = computed(() => formatLiteratureEntry(props.entry, formatDate));
</script>

<template>
    <template v-for="(segment, index) in segments" :key="index">
        <NuxtLink v-if="segment.href" :to="segment.href" target="_blank" class="break-all font-mono text-xs hover:text-primary">{{ segment.text }}</NuxtLink>
        <em v-else-if="segment.italic">{{ segment.text }}</em>
        <template v-else>{{ segment.text }}</template>
    </template>
</template>
