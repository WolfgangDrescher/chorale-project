<script setup>
// The line below the title: the collection, and Bach's id and BWV number.
const props = defineProps({
    chorale: { type: Object, required: true },
});

const isBach = computed(() => props.chorale.collection === BACH_CHORALES.id);
</script>

<template>
    <div class="flex flex-wrap items-baseline gap-x-1.5 text-xs text-muted">
        <span>{{ chorale.collectionName }}</span>
        <template v-if="isBach">
            <span aria-hidden="true">·</span>
            <span class="font-mono">{{ chorale.choraleId }}</span>
        </template>
        <template v-if="chorale.bwv">
            <span aria-hidden="true">·</span>
            <span>{{ chorale.bwv }}</span>
        </template>
    </div>
</template>
