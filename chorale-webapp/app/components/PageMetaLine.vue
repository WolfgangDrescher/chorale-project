<script setup>
defineProps({
    // { icon, text, title?, commit? }
    items: { type: Array, required: true },
});

const commitUrl = (commit) => `https://github.com/WolfgangDrescher/chorale-project/commit/${commit}`;
</script>

<template>
    <p v-if="items.length" class="mt-4 flex flex-wrap items-center gap-x-5 gap-y-1.5 text-sm text-muted">
        <span v-for="item in items" :key="item.icon" class="inline-flex items-center gap-1.5" :title="item.title">
            <UIcon :name="item.icon" class="size-4 shrink-0" />
            {{ item.text }}
            <NuxtLink v-if="item.commit" :to="commitUrl(item.commit)" target="_blank" class="rounded bg-elevated px-1 font-mono text-xs hover:text-primary">{{ item.commit.slice(0, 7) }}</NuxtLink>
        </span>
    </p>
</template>
