<script setup>
// The diminutions of the analysis layer of a chorale, as returned by the legend of
// useDiminutionAnalysis: color, name and number of notes, each shown or hidden with its checkbox.
defineProps({
    legend: { type: Array, required: true },
});

const emit = defineEmits(['toggle']);
</script>

<template>
    <ul class="flex flex-wrap gap-x-5 gap-y-2 text-sm">
        <li v-for="diminution in legend" :key="diminution.id">
            <UCheckbox :model-value="diminution.shown" @update:model-value="emit('toggle', diminution.id)">
                <template #label>
                    <span class="flex items-center gap-1.5">
                        <span class="size-4 shrink-0 rounded-full" :style="{ backgroundColor: highlightColorsByName[diminution.color] }" />
                        <span>{{ $t(`diminution.${diminution.id}`) }}</span>
                        <span class="text-dimmed tabular-nums">{{ diminution.count }}</span>
                    </span>
                </template>
            </UCheckbox>
        </li>
    </ul>
</template>
