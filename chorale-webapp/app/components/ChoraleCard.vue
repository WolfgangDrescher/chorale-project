<script setup>
// A chorale in a list of matches, with its player in the header and its score in the slot.
defineProps({
    choraleId: { type: String, required: true },
    chorale: { type: Object, default: undefined },
    matchCount: { type: Number, required: true },
    player: { type: Object, required: true },
    to: { type: Object, required: true },
});
</script>

<template>
    <UCard>
        <template #header>
            <div class="flex items-center justify-between gap-4">
                <div class="flex flex-wrap items-baseline gap-x-2">
                    <NuxtLink :to="to" class="flex flex-wrap items-baseline gap-x-2 hover:text-primary">
                        <span class="font-semibold">{{ choraleId }}</span>
                        <span v-if="chorale?.title">{{ chorale.title }}</span>
                    </NuxtLink>
                    <span v-if="chorale?.bwv" class="text-sm text-muted">{{ chorale.bwv }}</span>
                </div>
                <div class="flex items-center gap-2">
                    <UBadge color="neutral" variant="subtle" :label="$t('matchCount', matchCount)" />
                    <ScorePlayerControls :player="player" compact />
                </div>
            </div>
        </template>
        <slot />
    </UCard>
</template>
