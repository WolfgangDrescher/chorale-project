<script setup>
import { storeToRefs } from 'pinia';

const props = defineProps({
    scoreFormatter: {
        type: Object,
        required: true,
    },
});

const { scoreFilters } = storeToRefs(useKittelViewOptionsStore());
const { filters, addFilter, removeFilter } = props.scoreFormatter;

scoreFilters.value.forEach(filter => addFilter(filter));

watch(filters, (value) => {
    scoreFilters.value = [...value];
}, {deep: true});

function hasFilter(filter) {
    return filters.value.map(f => f.className).includes(filter.className);
}

const intervallsatzPresetFilter = new IntervallsatzPresetFilter();
const intervallsatzPreset = computed({
    get: () => hasFilter(intervallsatzPresetFilter),
    set: (value) => value ? addFilter(intervallsatzPresetFilter) : removeFilter(intervallsatzPresetFilter),
});

const hideFiguredbassFilter = new HideFiguredbassFilter();
const figuredbassFilter = computed({
    get: () => hasFilter(hideFiguredbassFilter),
    set: (value) => value ? addFilter(hideFiguredbassFilter) : removeFilter(hideFiguredbassFilter),
});

const bassScaleDegreeFilter = new BassScaleDegreeFilter();
const bassScaleDegree = computed({
    get: () => hasFilter(bassScaleDegreeFilter),
    set: (value) => value ? addFilter(bassScaleDegreeFilter) : removeFilter(bassScaleDegreeFilter),
});

const scaleDegreePresetFilter = new ScaleDegreePresetFilter();
const scaleDegreePreset = computed({
    get: () => hasFilter(scaleDegreePresetFilter),
    set: (value) => value ? addFilter(scaleDegreePresetFilter) : removeFilter(scaleDegreePresetFilter),
});

const bassSelector = ref('all');
watch(bassSelector, (value) => {
    if (value === 'all') {
        removeFilter(new ExtractSpineFilter());
    } else {
        const num = 8 - parseInt(value, 10) + 1;
        removeFilter(new ExtractSpineFilter());
        addFilter(new ExtractSpineFilter(`${num * 2 - 1},${num * 2},$`));
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
            <FormKittelBassSelector v-model="bassSelector" />
        </div>
        <div>
            <FormCheckbox v-model="intervallsatzPreset" :label="$t('humdrumFilter.IntervallsatzPresetFilter')" />
        </div>
        <div>
            <FormCheckbox v-model="figuredbassFilter" :label="$t('humdrumFilter.HideFiguredbassFilter')" />
        </div>
        <div>
            <FormCheckbox v-model="bassScaleDegree" :label="$t('humdrumFilter.BassScaleDegreeFilter')" />
        </div>
        <div>
            <FormCheckbox v-model="scaleDegreePreset" :label="$t('humdrumFilter.ScaleDegreePresetFilter')" />
        </div>
        <div>
            <FormCheckbox v-model="figuresWithoutSlashes" :label="$t('humdrumFilter.FiguresWithoutSlashesFilter')" />
        </div>
    </div>
</template>
