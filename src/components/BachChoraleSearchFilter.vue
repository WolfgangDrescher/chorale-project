<script setup>
const { t } = useI18n()

const { data: cadenceData } = await useAsyncData('/bach-cadences', () => queryContent('/bach-cadences').find())
const { data: choraleData } = await useAsyncData('/bach-370-chorales', () => queryContent('/bach-370-chorales').find())
const bachChorales = createBachChorales(choraleData.value, cadenceData.value);

const filter = useBachChoraleFilterStore();

function updateFilter(prop, value) {
    filter.update(prop, value);
}

function resetFilter() {
    filter.reset();
}

const keyOptions = [...new Set(bachChorales.map(chorale => chorale.key).filter(n => n))].sort((a, b) => {
    const notes = ['c', 'd', 'e', 'f', 'g', 'a', 'b'];
    const formatKey = (value) => value.replace(/\W/, '').toLowerCase();
    return notes.indexOf(formatKey(a)) - notes.indexOf(formatKey(b));
}).map(key => ({
    value: key,
    text: key,
}));

const majMinOptions = [
    { value: null, text: t('all') },
    { value: 'major', text: t('major') },
    { value: 'minor', text: t('minor') },
];
</script>

<template>
    <ClientOnly>
        <div class="grid grid-cols-4 gap-4 mb-4">
            <div>
                <FormInputField :model-value="filter.searchText" @update:model-value="updateFilter('searchText', $event)" :label="$t('searchText')" :placeholder="$t('searchText')" />
            </div>
            <div>
                <FormDropdown :model-value="filter.majorMinor" @update:model-value="updateFilter('majorMinor', $event)" :label="$t('majorMinor')" :options="majMinOptions" :search-enabled="false" />
            </div>
            <div>
                <FormDropdown :model-value="filter.keys" @update:model-value="updateFilter('keys', $event)" :label="$t('key')" :options="keyOptions" :search-enabled="false" :multiple="true"/>
            </div>
            <div>
                <FormRangeSlider :group-label="$t('countCadences')" :model-value="filter.countCadences" @update:model-value="updateFilter('countCadences', $event)" :min="0" :max="23" />
            </div>
            <div>
                <FormRangeSlider :group-label="$t('numberOfMeasures')" :model-value="filter.numberOfMeasures" @update:model-value="updateFilter('numberOfMeasures', $event)" :min="0" :max="48" />
            </div>
            <div>
                <FormGroup :label="''">
                    <FormButton @click="resetFilter" block>{{ $t('clear') }}</FormButton>
                </FormGroup>
            </div>
        </div>
    </ClientOnly>
</template>
