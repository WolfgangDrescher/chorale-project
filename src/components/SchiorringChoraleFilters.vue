<script setup>
import { storeToRefs } from 'pinia';

const props = defineProps({
    scoreFormatter: {
        type: Object,
        required: true,
    },
});

const { scoreFilters } = storeToRefs(useSchiorringViewOptionsStore());
const { filters, addFilter, removeFilter } = props.scoreFormatter;

scoreFilters.value.forEach(filter => addFilter(filter));

watch(filters, (value) => {
    scoreFilters.value = [...value];
}, {deep: true});

function hasFilter(filter) {
    return filters.value.map(f => f.className).includes(filter.className);
}

const modernClefsModoriFilter = new ModernClefsModoriFilter();
const modernClefsModori = computed({
    get: () => hasFilter(modernClefsModoriFilter),
    set: (value) => value ? addFilter(modernClefsModoriFilter) : removeFilter(modernClefsModoriFilter),
});

const hideFiguredbassFilter = new HideFiguredbassFilter();
const figuredbassFilter = computed({
    get: () => hasFilter(hideFiguredbassFilter),
    set: (value) => value ? addFilter(hideFiguredbassFilter) : removeFilter(hideFiguredbassFilter),
});

const partSelector = ref([]);
watch(partSelector, (value) => {
    removeFilter(new ExtractSpineFilter());
    const spines = [];
    if (value.includes('fb')) {
        spines.push('1-3');
    }
    if (value.includes('piano')) {
        spines.push('4-5');
    }
    if (value.includes('satb')) {
        spines.push('6-9');
    }
    if (spines.length) {
        addFilter(new ExtractSpineFilter(spines.join(',')));
    }
}, {immediate: true});

const figuresWithoutSlashesFilter = new FiguresWithoutSlashesFilter();
const figuresWithoutSlashes = computed({
    get: () => hasFilter(figuresWithoutSlashesFilter),
    set: (value) => value ? addFilter(figuresWithoutSlashesFilter) : removeFilter(figuresWithoutSlashesFilter),
});
</script>

<template>
    <div class="grid grid-cols-2 md:grid-cols-3 gap-4 items-center">
        <div>
            <FormDropdown v-model="partSelector" :label="$t('schiorringSelectPart')" :options="[
                {value: 'satb', text: $t('schiorringPartSatb')},
                {value: 'piano', text: $t('schiorringPartPiano')},
                {value: 'fb', text: $t('schiorringPartFb')},
            ]" :search-enabled="false" :multiple="true" />
        </div>
        <div>
            <FormCheckbox v-model="modernClefsModori" :label="$t('humdrumFilter.ModernClefsModoriFilter')" group-label="" />
        </div>
        <div>
            <FormCheckbox v-model="figuredbassFilter" :label="$t('humdrumFilter.HideFiguredbassFilter')" group-label="" />
        </div>
        <div>
            <FormCheckbox v-model="figuresWithoutSlashes" :label="$t('humdrumFilter.FiguresWithoutSlashesFilter')" group-label="" />
        </div>
    </div>
</template>
