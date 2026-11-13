<script setup>
// A chorale over the whole width with its score in one line, below a header that leads to its page.
defineProps({
    chorale: { type: Object, required: true },
    to: { type: String, required: true },
    visibleColumns: { type: Object, required: true },
    relativeColoring: { type: Boolean, default: false },
});
</script>

<template>
    <article class="rounded-lg border border-default overflow-hidden">
        <NuxtLink
            :to="to"
            class="group relative flex items-center gap-4 px-4 pt-3 bg-elevated/50 transition-colors hover:bg-elevated"
            :class="visibleColumns.modulations ? 'pb-4' : 'pb-3'"
        >
            <span class="w-10 text-3xl font-light leading-none tabular-nums text-dimmed group-hover:text-primary">{{ chorale.number }}</span>
            <div class="min-w-0 flex-1">
                <div class="font-medium text-highlighted group-hover:text-primary">{{ chorale.title }}</div>
                <ChoraleListItemDetails :chorale="chorale" />
            </div>
            <ChoraleMeter v-if="visibleColumns.meter && chorale.meters" :meters="chorale.meters" class="text-toned" />
            <ChoraleKeyBadge
                v-if="visibleColumns.key && startKeyOf(chorale)"
                :key-context="startKeyOf(chorale)"
                :cantus-firmus="differingCantusFirmusKeyOf(chorale)"
            />
            <ChoraleModulationMap
                v-if="visibleColumns.modulations && chorale.modulations && chorale.duration"
                :modulations="chorale.modulations"
                :duration="chorale.duration"
                :relative="relativeColoring"
                class="absolute inset-x-0 bottom-0 h-1"
            />
        </NuxtLink>
        <HighlightedScore
            :piece-id="chorale.choraleId"
            :horizontal="true"
            :verovio-options="{ scale: 35, pageMarginLeft: 70, header: 'none', footer: 'none' }"
        />
    </article>
</template>
