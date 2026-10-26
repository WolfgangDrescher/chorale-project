<script setup>
const props = defineProps({
    lesson: { type: Object, required: true },
});

const { t } = useI18n();

const names = props.lesson.analyses.map((analysis) => analysis.name);
const { data: results } = await useAsyncData(`analysis-summaries-${names.join('-')}`, () => {
    return queryCollection('analysisResults')
        .where('stem', 'IN', names.map((name) => `analysis-results/${name}`))
        .select('stem', 'matchCount')
        .all();
});

const cards = computed(() => props.lesson.analyses.map((analysis) => {
    const result = results.value?.find((entry) => entry.stem === `analysis-results/${analysis.name}`);
    return {
        value: analysis.name,
        label: analysis.title ?? analysis.name,
        description: result ? t('analysisMatchesCount', { n: result.matchCount }) : '',
    };
}));
const selected = ref(names[0]);
const analysis = computed(() => props.lesson.analyses.find((entry) => entry.name === selected.value));
</script>

<template>
    <div>
        <p v-if="lesson.analysesDescription" class="leading-7 text-pretty">{{ lesson.analysesDescription }}</p>
        <URadioGroup
            v-if="cards.length > 1"
            v-model="selected"
            :items="cards"
            variant="card"
            orientation="horizontal"
            indicator="hidden"
            class="mt-6"
            :ui="{ description: 'text-xs' }"
        />
        <Suspense>
            <AnalysisView
                :key="analysis.name"
                :name="analysis.name"
                :view="analysis.view"
                :colors="analysis.colors"
                :labels="analysis.labels"
            />
        </Suspense>
        <template v-if="lesson.queries?.length">
            <h2 class="mt-10 mb-3 text-lg font-semibold text-highlighted">{{ t('lessonQueries') }}</h2>
            <div class="-mx-0.5 flex flex-wrap gap-2">
                <QueryRef v-for="query in lesson.queries" :id="query.id" :key="query.id" :label="query.title" />
            </div>
        </template>
    </div>
</template>
