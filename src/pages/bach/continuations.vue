<script setup>
import { onKeyStroke } from '@vueuse/core';

definePageMeta({
    layout: 'bach',
});

const { t } = useI18n();
const localePath = useLocalePath();

useHead({
    title: t('continuations'),
});

const { data: phrasesData } = await useAsyncData('/bach-phrases', () => queryContent('/bach-phrases').find())
const { data: choraleData } = await useAsyncData('/bach-370-chorales', () => queryContent('/bach-370-chorales').find())
const chorales = createBachChorales(choraleData.value, phrasesData.value);
const { filteredElements } = useBachChoraleFilter(chorales);

const phrases = computed(() => {
    return filteredElements.value.map(c => c.phrases.filter(p => {
        const filterFb = (fb) => {
            if (!fb || !fb.length) return true;
            return fb.includes(p.continuation.fb);
        };
        const filterMint = (mint) => {
            if (!mint || !mint.length) return true;
            return mint.includes(p.continuation.mint);
        };
        return p.continuation && filterFb(options.fb) && filterMint(options.mint);
    })).flat();
});

const totalPhrases = computed(() => {
    return chorales.reduce((accumulator, chorale) => accumulator + chorale.phrases.filter(p => p.continuation).length, 0);
});

const options = reactive({
    mode: 'mint', // mint, fb
    mint: [],
    fb: [],
});

const phrasesCount = computed(() => {
    return Object.entries(phrases.value.reduce((obj, phrases) => {
        let index = phrases.continuation.mint;
        if (options.mode === 'fb') {
            index = phrases.continuation.fb;
        }
        obj[index] = (obj[index] ?? 0) + 1;
        return obj;
    }, {})).sort(([aKey], [bKey]) => {
        return sortPhraseDegrees(aKey, bKey);
    });
});

const groupedPhrases = computed(() => {
    return Object.entries(phrases.value.reduce((accumulator, phrase) => {
        accumulator[phrase.choraleId] = accumulator[phrase.choraleId] ?? [];
        accumulator[phrase.choraleId].push(phrase);
        return accumulator;
    }, {})).map(([choraleId, phrases]) => ({
        choraleId,
        phrases,
    }));
});

const config = computed(() => ({
    type: 'bar',
    data: {
        datasets: [{
            label: options.mode === 'fb' ? t('fbNumbers') : t('melodicIntervals'),
            data: phrasesCount.value.map(i => ({ x: i[0], y: i[1] })),
        }],
    },
    options: {
        interaction: {
            mode: 'nearest',
            axis: 'x',
            intersect: false,
        },
        responsive: true,
        maintainAspectRatio: false,
        plugins: {
            legend: {
                // display: false,
            },
            tooltip: {
                yAlign: 'bottom',
            },
        },
        scales: {
            y: {
                beginAtZero: true,
                ticks: {
                    precision: 0,
                },
            },
        },
    },
}));

const mintOptions = [...new Set(phrases.value.map(i => i.continuation.mint).filter(n => n))].sort().map((fb) => ({
    text: fb,
    value: fb,
}));

const fbOptions = [...new Set(phrases.value.map(i => i.continuation.fb).filter(n => n))].sort().map((fb) => ({
    text: fb,
    value: fb,
}));

let chartInstance = null;
function onChartMounted(chart) {
    chartInstance = chart;
}

function chartClickHandler(event) {
    const points = chartInstance.getElementsAtEventForMode(event, 'nearest', { intersect: false, axis: 'x' }, true);
    if (points.length) {
        const firstPoint = points[0];
        // const label = chartInstance.data.labels[firstPoint.index];
        const value = chartInstance.data.datasets[firstPoint.datasetIndex].data[firstPoint.index];
        if (options.mode === 'mint') {
            options.mint = [value?.x];
            options.fb = [];
            options.mode = 'fb';
        } else if (options.mode === 'fb') {
            options.mint = [];
            options.fb = [value?.x];
            options.mode = 'mint';
        }
    }
}

const openModal = ref(null);

function closeModal() {
    openModal.value = null
    activeIndex = null;
}

let activeIndex = null;
function loadIndex(index) {
    const item = groupedPhrases.value[index];
    if (item) {
        activeIndex = index;
        openModal.value = item.choraleId;
    }
}

onKeyStroke('ArrowLeft', () => {
    if (activeIndex !== null) loadIndex(activeIndex - 1);
});

onKeyStroke('ArrowRight', () => {
    if (activeIndex !== null) loadIndex(activeIndex + 1);
});
</script>

<template>
    <Container>

        <Heading>{{ $t('continuations') }}</Heading>

        <BachChoraleSearchFilter />

        <div class="grid grid-cols-4 gap-4 mb-4">
            <div>
                <FormDropdown v-model="options.mode" :label="$t('mode')" :options="[{ value: 'mint', text: t('melodicIntervals') }, { value: 'fb', text: t('fbNumbers') }]" :search-enabled="false" :multiple="false" />
            </div>
            <div>
                <FormDropdown v-model="options.mint" :label="$t('melodicIntervals')" :options="mintOptions" :search-enabled="false" :multiple="true" />
            </div>
            <div>
                <FormDropdown v-model="options.fb" :label="$t('fbNumbers')" :options="fbOptions" :search-enabled="false" :multiple="true" />
            </div>
        </div>

        <div class="my-4 flex flex-col md:flex-row gap-4">
            <div class="flex items-center">
                {{ $t('nOutOfTotalPhrasesFoundForSerachParams', { n: phrases.length, total: totalPhrases }) }}
            </div>
        </div>

        <div class="aspect-w-5 aspect-h-2">
            <Chart :config="config" @chart-mounted="onChartMounted" @click="chartClickHandler" />
        </div>

        <IntervalQualityLegend v-if="options.mode === 'mint'" direction rest />
        <FiguredBassLegend v-else-if="options.mode === 'fb'" negative null-token />

        <Badge v-for="(group, index) in groupedPhrases" :key="group.choraleId" @click="loadIndex(index)">
            {{ `${group.choraleId} (${group.phrases.length})` }}
            <Modal v-if="openModal === group.choraleId" @close="closeModal" :title="group.choraleId">
                <template v-slot:title>
                    <NuxtLink :href="localePath({ name: 'bach-chorale-nr', params: { nr: parseInt(group.choraleId.replaceAll(/\D/g, ''), 10) } })">
                        {{ group.choraleId }}
                    </NuxtLink>
                </template>
                <div class="grid grid-cols-2 gap-4">
                    <div v-for="phrase in group.phrases">
                        <VerovioCanvas :url="`/kern/bach-phrases/${phrase.continuation.filename}`" :scale="35" :options="{mnumInterval: 1}" :page-margin="50" />
                    </div>
                </div>
                <div class="flex gap-4">
                    <FormButton v-if="groupedPhrases[index - 1]" @click="loadIndex(index - 1)" class="mr-auto">{{ $t('previous') }}</FormButton>
                    <FormButton v-if="groupedPhrases[index + 1]" @click="loadIndex(index + 1)" class="ml-auto">{{ $t('next') }}</FormButton>
                </div>
            </Modal>
        </Badge>

    </Container>
</template>
