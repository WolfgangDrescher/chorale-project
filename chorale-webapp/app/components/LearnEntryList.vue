<script setup>
defineProps({
    entries: { type: Array, required: true },
});
</script>

<template>
    <ul class="divide-y divide-default rounded-lg border border-default">
        <li v-for="entry in entries" :key="entry.path">
            <NuxtLink :to="entry.path" class="group flex gap-4 p-4 hover:bg-elevated/60">
                <slot :entry="entry" />
            </NuxtLink>
        </li>
    </ul>
</template>
