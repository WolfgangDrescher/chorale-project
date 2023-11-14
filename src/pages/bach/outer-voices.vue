<script setup>
import { getResolvedTokenLineIndex } from '../../../scripts/utils/humdrum.mjs';
import { onKeyStroke } from '@vueuse/core';

definePageMeta({
    layout: 'bach',
});

const { t } = useI18n();

useHead({
    title: t('phraseDegrees'),
});

const { data: outerVoicesData } = await useAsyncData('/bach-outer-voices', () => queryContent('/bach-outer-voices').findOne())
const { data: phrasesData } = await useAsyncData('/bach-phrases', () => queryContent('/bach-phrases').find())
const { data: choraleData } = await useAsyncData('/bach-370-chorales', () => queryContent('/bach-370-chorales').find())
const chorales = createBachChorales(choraleData.value, phrasesData.value);
const { filteredElements } = useBachChoraleFilter(chorales);

const totalSlices = outerVoicesData.value.body.length;

const options = reactive({
    mode: 'intervals', // intervals, fb, hint
    beat: '', // 1, 0.5, 0.5!1, 0.25!0.5
    position: null, // phraseStart, phraseEnd, choraleStart, choraleEnd
    fb: [],
});

const intervals = computed(() => {
    return outerVoicesData.value.body.filter(e => {
        if (!filteredElements.value.map(c => c.id).includes(e.choraleId)) return false;
        const filterBeat = (beat) => {
            if (!beat) return true;
            if (beat == '0.5!1') return e.beat % 0.5 === 0 && e.beat % 1 !== 0;
            if (beat == '0.25!0.5') return e.beat % 0.25 === 0 && e.beat % 0.5 !== 0;
            return e.beat % parseFloat(beat) === 0;
        };
        const filterPosition = (position) => {
            switch (position) {
                case 'phraseStart':
                    return e.isPhraseStart;
                case 'phraseEnd':
                    return e.isPhraseEnd;
                case 'choraleStart':
                    return e.isChoraleStart;
                case 'choraleEnd':
                    return e.isChoraleEnd;
            }
            return true;
        };
        const filterFb = (fb) => {
            if (!fb || !fb.length) return true;
            return fb.includes(e.fb) || fb.includes(e.hint);
        };
        return filterBeat(options.beat) && filterPosition(options.position) && filterFb(options.fb);
    });
});

const limitedIntervals = computed(() => {
    return intervals.value.slice(0, 500);
});

const intervalCount = computed(() => {
    return Object.entries(intervals.value.reduce((obj, interval) => {
        let index = interval.fb.replaceAll(/\D/g, '');
        if (options.mode === 'fb') {
            index = interval.fb;
        } else if (options.mode === 'hint') {
            index = interval.hint;
        }
        obj[index] = (obj[index] ?? 0) + 1;
        return obj;
    }, {})).sort(([aKey], [bKey]) => {
        return bKey.replaceAll(/\D/g, '') < aKey.replaceAll(/\D/g, '') ? 1 : -1;
    });
});

const config = computed(() => ({
    type: 'bar',
    data: {
        datasets: [{
            data: intervalCount.value.map(i => ({ x: i[0], y: i[1] }))
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
                display: false,
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

const fbOptions = [...new Set(intervals.value.map(i => i.fb.replaceAll(/\D/g, '')).filter(n => n))].sort().map((fb) => ({
    text: fb,
    value: fb,
}));

let chartInstance = null;
function onChartMounted(chart) {
    chartInstance = chart;
}

function chartClickHandler(event) {
    const points = chartInstance.getElementsAtEventForMode(event, 'nearest', { intersect: true }, true);
    if (points.length) {
        const firstPoint = points[0];
        // const label = chartInstance.data.labels[firstPoint.index];
        const value = chartInstance.data.datasets[firstPoint.datasetIndex].data[firstPoint.index];
        const intValue = value?.x?.replaceAll(/\D/g, '');
        if (fbOptions.map(f => f.value).includes(intValue)) {
            options.fb = options.fb.includes(value?.x) ? [] : [value?.x];
        }
    }
}

const openModal = ref(null);
const modalScoreData = ref();
async function loadScoreData(interval) {
    modalScoreData.value = null;
    const response = await $fetch(`/bach-370-chorales/${interval.choraleId}.krn`);
    const data = await  response.text();
    const lines = data.split('\n');
    for (let i = 0; i < lines.length; i++) {
        if (i === interval.lineIndex) {
            [0, 3].forEach((tokenIndex) => {
                const resolvedLineIndex = getResolvedTokenLineIndex(i, tokenIndex, lines);
                lines[resolvedLineIndex] = lines[resolvedLineIndex].split('\t').map((token, ti) => {
                    if (ti === tokenIndex) {
                        return `${token}@`;
                    }
                    return token;
                }).join('\t');
            });
        }
    }
    lines.push('!!!filter: extract -k 1,4 | rid -glid | fb -ci');
    lines.push('!!!RDF**kern: @ = marked note');
    modalScoreData.value = lines.join('\n');
    openModal.value = `${interval.choraleId}${interval.lineIndex}`;
}

function closeModal() {
    openModal.value = null
    activeIndex = null;
    modalScoreData.value = null;
}

let activeIndex = null;
function loadIndex(index) {
    const item = limitedIntervals.value[index]; 
    if (item) {
        loadScoreData(item);
        activeIndex = index;
    }
}

onKeyStroke('ArrowLeft', () => {
    if (activeIndex !== null) loadIndex(activeIndex - 1)
});

onKeyStroke('ArrowRight', () => {
    if (activeIndex !== null) loadIndex(activeIndex + 1)
});

</script>

<template>
    <Container>

        <Heading>{{ $t('outerVoices') }}</Heading>

        <BachChoraleSearchFilter />

        <div class="grid grid-cols-4 gap-4 mb-4">
            <div>
                <FormDropdown v-model="options.mode" :label="$t('intervalMode')" :options="[{ value: 'intervals', text: t('intervals')}, { value: 'fb', text: t('figuresbass') }, { value: 'hint', text: t('harmonicIntervals') }]" :search-enabled="false" :multiple="false" />
            </div>
            <div>
                <FormDropdown v-model="options.beat" :label="$t('rhythmicPosition')" :options="[{ value: '1', text: t('quarterNotes') }, { value: '0.5', text: t('eighthNotes') }, { value: '0.5!1', text: t('eighthNotesNotQuater') }, { value: '0.25!0.5', text: t('sixteenthNotesNotEighth') }]" :search-enabled="false" :multiple="false" />
            </div>
            <div>
                <FormDropdown v-model="options.position" :label="$t('intervalPosition')" :options="[{ value: 'phraseStart', text: t('phraseStart') }, { value: 'phraseEnd', text: t('phraseEnd') }, { value: 'choraleStart', text: t('choraleStart') }, { value: 'choraleEnd', text: t('choraleEnd') }]" :search-enabled="false" :multiple="false" />
            </div>
            <div>
                <FormDropdown v-model="options.fb" :label="$t('fbNumbers')" :options="fbOptions" :search-enabled="false" :multiple="true" />
            </div>
        </div>

        <div class="my-4 flex flex-col md:flex-row gap-4">
            <div class="flex items-center">
                {{ $t('nOutOfTotalIntervalsFoundForSerachParams', { n: intervals.length, total: totalSlices }) }}
            </div>
        </div>

        <div class="aspect-w-5 aspect-h-2">
            <Chart :config="config" @chart-mounted="onChartMounted" @click="chartClickHandler" />
        </div>

        <Badge v-for="(interval, index) in limitedIntervals" :key="`${interval.choraleId}${interval.lineIndex}`" @click="loadIndex(index)">
            {{ `${parseInt(interval.choraleId.replaceAll(/\D/g, ''), 10)}/${interval.lineIndex + 1}` }}
            <Modal v-if="openModal === `${interval.choraleId}${interval.lineIndex}`" @close="closeModal" :title="`${interval.choraleId} / ${interval.lineIndex + 1}`">
                <VerovioCanvas :data="modalScoreData" :scale="35" :page-margin="50" />
                <div class="flex gap-4">
                    <FormButton v-if="limitedIntervals[index - 1]" @click="loadIndex(index - 1)" ref="prev" class="mr-auto">{{ $t('previous') }}</FormButton>
                    <FormButton v-if="limitedIntervals[index + 1]" @click="loadIndex(index + 1)" ref="next" class="ml-auto">{{ $t('next') }}</FormButton>
                </div>
            </Modal>
        </Badge>

        <Badge v-if="limitedIntervals.length < intervals.length">{{ $t('nMore', intervals.length - limitedIntervals.length) }}</Badge>

    </Container>
</template>
