<script setup>
const props = defineProps({
    // The analysis, by its file in analyses/ without .json.
    name: { type: String, required: true },
    // The chart above the table: a bar per group, or the groups of the first level stacked in one bar.
    view: { type: String, default: 'bars' },
    // The colors of the stacked bar, by the label of the group: { perfect: primary-600 }.
    colors: { type: Object, default: () => ({}) },
    labels: { type: Array, default: () => [] },
});

const { t } = useI18n();

const { data: result } = await useAsyncData(`analysis-${props.name}`, () => {
    return queryCollection('analysisResults').where('stem', '=', `analysis-results/${props.name}`).first();
});

provide('analysisFormat', useAnalysisFormat(result));

const PHRASES = ['phraseStart', 'phraseMiddle', 'phraseEnd'];
const phrases = computed(() => PHRASES.filter((name) => result.value?.variants?.[name]));
const phrase = ref('all');
const phraseItems = computed(() => [
    { label: t('analysisAllMatches'), value: 'all' },
    ...phrases.value.map((name) => ({ label: t(`analysisVariant.${name}`), value: name })),
]);

const hasIgnoreQuality = computed(() => !!result.value?.variants?.ignoreQuality);
const ignoreQuality = ref(false);

// Both choices together are a variant of their own, named after both ("phraseEnd+ignoreQuality").
const variant = computed(() => {
    const names = [phrase.value !== 'all' && phrase.value, ignoreQuality.value && hasIgnoreQuality.value && 'ignoreQuality'].filter(Boolean);
    const name = names.join('+');
    return !name || result.value?.variants?.[name] ? name : names[0];
});
const analysis = computed(() => (variant.value ? result.value?.variants?.[variant.value] : result.value));

// The gray bar of the rest opens the table to all of its rows. Another variant starts with the first ones.
const showAll = ref(false);
watch(variant, () => {
    showAll.value = false;
});
const table = useTemplateRef('table');

async function showMore() {
    showAll.value = true;
    await nextTick();
    // Down to the end of the rows, which shows the ones new to it.
    table.value?.$el?.querySelector?.('tfoot')?.scrollIntoView({ behavior: 'smooth', block: 'end' });
}
</script>

<template>
    <div class="not-prose my-6">
        <p v-if="!result" class="text-sm text-muted">{{ t('analysisMissing') }}</p>
        <template v-else>
            <AnalysisStackedBar v-if="view === 'stacked'" :result="result" :phrases="phrases" :colors="colors" class="mb-8" />
            <div v-if="phrases.length || hasIgnoreQuality" class="mb-4 flex flex-wrap items-center gap-x-6 gap-y-3">
                <UTabs v-if="phrases.length" v-model="phrase" :items="phraseItems" :content="false" size="md" />
                <USwitch v-if="hasIgnoreQuality" v-model="ignoreQuality" :label="t('ignoreIntervalQualityShort')" />
            </div>
            <AnalysisBarChart v-if="view === 'bars'" :key="`bars-${variant}`" :result="result" :analysis="analysis" @more="showMore" />
            <AnalysisTable ref="table" :key="`table-${variant}`" v-model:show-all="showAll" :analysis="analysis" :labels="labels" />
        </template>
    </div>
</template>
