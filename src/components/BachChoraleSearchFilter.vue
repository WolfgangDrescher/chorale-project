<script setup>
import bachChoraleFilterOptions from '../utils/bach-chorale-filter-options.json';

const { t } = useI18n()

const filter = useBachChoraleFilterStore();

function updateFilter(prop, value) {
    filter.update(prop, value);
}

function resetFilter() {
    filter.reset();
}

const keyOptions = bachChoraleFilterOptions.keys.sort((a, b) => {
    const notes = ['c', 'd', 'e', 'f', 'g', 'a', 'b'];
    const formatKey = (value) => value.replace(/\W/, '').toLowerCase();
    return notes.indexOf(formatKey(a)) - notes.indexOf(formatKey(b));
}).map(key => ({
    value: key,
    text: key,
}));

const timeSignatureOptions = bachChoraleFilterOptions.timeSignatures.sort((a, b) => {
    const [numeratorA, denominatorA] = a.split('/');
    const [numeratorB, denominatorB] = b.split('/');
    if (denominatorA < denominatorB) return 1;
    if (denominatorA > denominatorB) return -1;
    if (numeratorA < numeratorB) return 1;
    if (numeratorA > numeratorB) return -1;
}).map(timeSignature => ({
    value: timeSignature,
    text: timeSignature,
}));

const majMinOptions = [
    { value: null, text: t('all') },
    { value: 'major', text: t('major') },
    { value: 'minor', text: t('minor') },
];

const cadenceDegreeOptions = bachChoraleFilterOptions.degrees.sort(sortPhraseDegrees).map(degree => ({
    value: degree,
    text: romanizeDeg(degree),
}));

const cadenceDegreeFbNumberOptions = bachChoraleFilterOptions.fbNumbers.sort().map(fb => ({
    value: fb,
    text: fb,
}));
</script>

<template>
    <ClientOnly>
        <div class="grid grid-cols-2 md:grid-cols-3 lg:grid-cols-4 gap-4 mb-4">
            <div>
                <FormInputField :model-value="filter.searchText" @update:model-value="updateFilter('searchText', $event)" :label="$t('searchText')" :placeholder="$t('searchText')" />
            </div>
            <div>
                <FormDropdown :model-value="filter.majorMinor" @update:model-value="updateFilter('majorMinor', $event)" :label="$t('majorMinor')" :options="majMinOptions" :search-enabled="false" />
            </div>
            <div>
                <FormDropdown :model-value="filter.keys" @update:model-value="updateFilter('keys', $event)" :label="$t('key')" :options="keyOptions" :search-enabled="false" :multiple="true" />
            </div>
            <div>
                <FormDropdown :model-value="filter.timeSignature" @update:model-value="updateFilter('timeSignature', $event)" :label="$t('timeSignature')" :options="timeSignatureOptions" :search-enabled="false" :multiple="false" />
            </div>
            <div>
                <FormDropdown :model-value="filter.phraseDegrees" @update:model-value="updateFilter('phraseDegrees', $event)" :label="$t('phraseDegrees')" :options="cadenceDegreeOptions" :search-enabled="false" :multiple="true" />
            </div>
            <div>
                <FormDropdown :model-value="filter.phraseFbNumbers" @update:model-value="updateFilter('phraseFbNumbers', $event)" :label="$t('phraseFbNumbers')" :options="cadenceDegreeFbNumberOptions" :search-enabled="false" :multiple="true" />
            </div>
            <div>
                <FormRangeSlider :group-label="$t('countPhrases')" :model-value="filter.countPhrases" @update:model-value="updateFilter('countPhrases', $event)" :min="0" :max="23" />
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
