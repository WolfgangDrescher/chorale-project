<script setup>
const props = defineProps({
    scoreFormatter: {
        type: Object,
        required: true,
    },
});

const { addFilter, removeFilter } = props.scoreFormatter;

const satb2gs = ref(false);
const satb2gsFilter = new Satb2gsFilter();
watch(satb2gs, (value) => {
    if (value) {
        addFilter(satb2gsFilter);
    } else {
        removeFilter(satb2gsFilter.id);
    }
});

const intervallsatzPreset = ref(false);
const intervallsatzPresetFilter = new IntervallsatzPresetFilter();
watch(intervallsatzPreset, (value) => {
    if (value) {
        addFilter(intervallsatzPresetFilter);
    } else {
        removeFilter(intervallsatzPresetFilter.id);
    }
});

const extendedFiguredbassPreset = ref(false);
const extendedFiguredbassPresetFilter = new ExtendedFiguredbassPresetFilter();
watch(extendedFiguredbassPreset, (value) => {
    if (value) {
        addFilter(extendedFiguredbassPresetFilter);
    } else {
        removeFilter(extendedFiguredbassPresetFilter.id);
    }
});

const bassScaleDegree = ref(false);
const bassScaleDegreeFilter = new BassScaleDegreeFilter();
watch(bassScaleDegree, (value) => {
    if (value) {
        addFilter(bassScaleDegreeFilter);
    } else {
        removeFilter(bassScaleDegreeFilter.id);
    }
});

const scaleDegreePreset = ref(false);
const scaleDegreePresetFilter = new ScaleDegreePresetFilter();
watch(scaleDegreePreset, (value) => {
    if (value) {
        addFilter(scaleDegreePresetFilter);
    } else {
        removeFilter(scaleDegreePresetFilter.id);
    }
});

const hideMiddleVoices = ref(false);
const hideMiddleVoicesFilter = new HideMiddleVoicesFilter();
watch(hideMiddleVoices, (value) => {
    if (value) {
        addFilter(hideMiddleVoicesFilter);
    } else {
        removeFilter(hideMiddleVoicesFilter.id);
    }
});
</script>

<template>
    <div class="grid grid-cols-3 gap-4 items-center">
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
    </div>
</template>
