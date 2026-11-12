<script setup>
// The line below the title of a chorale: its id and its BWV number.
defineProps({
    chorale: { type: Object, required: true },
});
</script>

<template>
    <div class="flex flex-wrap items-baseline gap-x-1.5 text-xs text-muted">
        <span class="font-mono">{{ chorale.choraleId }}</span>
        <template v-if="chorale.bwv">
            <span aria-hidden="true">·</span>
            <span>{{ chorale.bwv }}</span>
        </template>
    </div>
</template>
