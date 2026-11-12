<script setup>
defineProps({
    chorale: { type: Object, required: true },
    to: { type: String, required: true },
    visibleColumns: { type: Object, required: true },
    relativeColoring: { type: Boolean, default: false },
});
</script>

<template>
    <NuxtLink :to="to" class="group flex flex-col gap-2 rounded-lg border border-default p-4 transition hover:border-primary hover:shadow-sm">
        <div class="flex items-center justify-between gap-2">
            <span class="text-3xl font-light leading-none tabular-nums text-dimmed group-hover:text-primary">{{ chorale.number }}</span>
            <div class="flex items-center gap-2">
                <ChoraleMeter v-if="visibleColumns.meter && chorale.meters" :meters="chorale.meters" class="text-toned" />
                <ChoraleKeyBadge
                    v-if="visibleColumns.key && startKeyOf(chorale)"
                    :key-context="startKeyOf(chorale)"
                    :cantus-firmus="differingCantusFirmusKeyOf(chorale)"
                />
            </div>
        </div>
        <div>
            <div class="font-medium leading-snug text-highlighted line-clamp-2 group-hover:text-primary">{{ chorale.title }}</div>
            <ChoraleListItemDetails :chorale="chorale" class="mt-0.5" />
        </div>
        <ChoraleIncipit v-if="visibleColumns.incipit" :chorale-id="chorale.choraleId" class="h-14 mt-auto" />
        <ChoraleModulationMap
            v-if="visibleColumns.modulations && chorale.modulations && chorale.duration"
            :modulations="chorale.modulations"
            :duration="chorale.duration"
            :relative="relativeColoring"
            class="h-1.5 rounded-sm"
            :class="!visibleColumns.incipit && 'mt-auto'"
        />
    </NuxtLink>
</template>
