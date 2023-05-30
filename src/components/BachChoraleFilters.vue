<script setup>
import { storeToRefs } from 'pinia';

const props = defineProps({
    scoreFormatter: {
        type: Object,
        required: true,
    },
});

const { scoreFilters } = storeToRefs(useBachViewOptionsStore());
const { filters, addFilter, removeFilter } = props.scoreFormatter;

scoreFilters.value.forEach(filter => addFilter(filter));

watch(filters, (value) => {
    scoreFilters.value = [...value];
}, {deep: true});

function hasFilter(filter) {
    return scoreFilters.value.map(f => f.className).includes(filter.className);
}

const satb2gsFilter = new Satb2gsFilter();
const satb2gs = computed({
    get: () => hasFilter(satb2gsFilter),
    set: (value) => value ? addFilter(satb2gsFilter) : removeFilter(satb2gsFilter),
});

const intervallsatzPresetFilter = new IntervallsatzPresetFilter();
const intervallsatzPreset = computed({
    get: () => hasFilter(intervallsatzPresetFilter),
    set: (value) => value ? addFilter(intervallsatzPresetFilter) : removeFilter(intervallsatzPresetFilter),
});

const extendedFiguredbassPresetFilter = new ExtendedFiguredbassPresetFilter();
const extendedFiguredbassPreset = computed({
    get: () => hasFilter(extendedFiguredbassPresetFilter),
    set: (value) => value ? addFilter(extendedFiguredbassPresetFilter) : removeFilter(extendedFiguredbassPresetFilter),
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

const hideMiddleVoicesFilter = new HideMiddleVoicesFilter();
const hideMiddleVoices = computed({
    get: () => hasFilter(hideMiddleVoicesFilter),
    set: (value) => value ? addFilter(hideMiddleVoicesFilter) : removeFilter(hideMiddleVoicesFilter),
});

const extractCantusFirmusFilter = new ExtractCantusFirmusFilter();
const extractCantusFirmus = computed({
    get: () => hasFilter(extractCantusFirmusFilter),
    set: (value) => value ? addFilter(extractCantusFirmusFilter) : removeFilter(extractCantusFirmusFilter),
});
</script>

<template>
    <div class="grid grid-cols-2 md:grid-cols-3 gap-4 items-center">
        <div>
            <FormCheckbox v-model="satb2gs" :label="$t('humdrumFilter.Satb2gsFilter')" />
        </div>
        <div>
            <FormCheckbox v-model="intervallsatzPreset" :label="$t('humdrumFilter.IntervallsatzPresetFilter')" />
        </div>
        <div>
            <FormCheckbox v-model="extendedFiguredbassPreset" :label="$t('humdrumFilter.ExtendedFiguredbassPresetFilter')" />
        </div>
        <div>
            <FormCheckbox v-model="bassScaleDegree" :label="$t('humdrumFilter.BassScaleDegreeFilter')" />
        </div>
        <div>
            <FormCheckbox v-model="scaleDegreePreset" :label="$t('humdrumFilter.ScaleDegreePresetFilter')" />
        </div>
        <div>
            <FormCheckbox v-model="hideMiddleVoices" :label="$t('humdrumFilter.HideMiddleVoicesFilter')" />
        </div>
        <div>
            <FormCheckbox v-model="extractCantusFirmus" :label="$t('humdrumFilter.ExtractCantusFirmusFilter')" />
        </div>
    </div>
</template>
