<script setup>
definePageMeta({
    layout: 'bach',
});

// import { gray } from 'tailwindcss/colors';
const gray50 = '#f9fafb'; // gray[50] will not work with nuxt generate

const { t } = useI18n();
const localePath = useLocalePath();

useHead({
    title: t('phraseDegrees'),
});

const { data: phrasesData } = await useAsyncData('/bach-phrases', () => queryContent('/bach-phrases').find())
const { data: choraleData } = await useAsyncData('/bach-370-chorales', () => queryContent('/bach-370-chorales').find())
const chorales = createBachChorales(choraleData.value, phrasesData.value);
const { filteredElements } = useBachChoraleFilter(chorales);


const maxCadences = computed(() => {
    if (filteredElements.value.length === 1) {
        return filteredElements.value[0].countPhrases
    } else if (filteredElements.value.length >= 2) {
        const maxCadenceItem = filteredElements.value?.reduce((prev, current) => {
            return (prev && prev.countPhrases > current.countPhrases) ? prev : current;
        });
        return maxCadenceItem?.countPhrases;
    }
    return 0;
});

const tableHeaders = computed(() => {
    return [{
        text: t('title'),
        value: 'id',
        align: 'center',
        cellBgColor: gray50,
    }, ...Array.from({ length: maxCadences.value }, (_, i) => i + 1).map((n) => ({ text: n, value: n, align: 'center' }))];
});

const tableItems = computed(() => {
    return filteredElements.value.map(chorale => {
        const result = {
            id: chorale.id,
            title: chorale.fullTitle,
            nr: chorale.nr,
        };
        chorale.phrases.forEach((phrase, index) => {
            result[index + 1] = romanizeDeg(phrase.degree);
            result[`${index + 1}.fb`] = phrase.fb;
            result[`${index + 1}.phraseId`] = phrase.id;
            result[`${index + 1}.filename`] = phrase.filename;
        });
        return result;

    });
});

const totalTableHeaders = computed(() => {
    const degreeItems = filteredElements.value.reduce((accumulator, chorale) => {
        chorale.phrases.forEach(cadence => {
            if (!accumulator.includes(cadence.degree)) {
                accumulator.push(cadence.degree);
            }
        });
        return accumulator;
    }, []).sort(sortPhraseDegrees).map(n => ({ text: romanizeDeg(n), value: n, align: 'center' }))
    return [{ text: t('fbFigure'), value: 'fbFigure', align: 'center' }, ...degreeItems, { text: t('total'), value: 'total', align: 'center', cellBgColor: gray50 }];
});
const totalTableItems = computed(() => {
    const result = [];

    // Count cadences for each fb figure and for each degree
    const fbFigure = [...new Set(filteredElements.value.map(chorale => chorale.phrases.map(c => c.fb)).flat())].sort();
    fbFigure.forEach((fbFig) => {
        const items = totalTableHeaders.value.map(({ value: n }) => {
            const count = filteredElements.value.reduce((accumulator, chorale) => {
                const num = chorale.phrases.filter(cadence => cadence.degree === n && fbFig === cadence.fb).length;
                return accumulator + num;
            }, 0);
            return [n, count];
        });
        const fbFigObj = Object.fromEntries(items)
        fbFigObj.fbFigure = fbFig;
        fbFigObj.total = items.reduce((accumulator, [, a]) => accumulator + a, 0)
        result.push(fbFigObj);
    });

    // Count all cadences for each degree
    const items = totalTableHeaders.value.map(({ value: n }) => {
        const count = filteredElements.value.reduce((accumulator, chorale) => {
            const num = chorale.phrases.filter(cadence => cadence.degree === n).length;
            return accumulator + num;
        }, 0);
        return [n, count];
    });
    const totalObj = Object.fromEntries(items)
    totalObj.fbFigure = t('total');
    totalObj.total = items.reduce((accumulator, [,a]) => accumulator + a, 0)
    totalObj._rowBgColor = gray50;
    result.push(totalObj)

    return result;
});

const openModal = ref(null);
const modalScoreData = ref('');
async function loadScoreData(filename) {
    modalScoreData.value = '';
    modalScoreData.value = await $fetch(`/kern/bach-phrases/${filename}`, { parseResponse: (txt) => txt });
}
</script>

<template>
    <Container>

        <Heading>{{ $t('phraseDegrees') }}</Heading>

        <BachChoraleSearchFilter />

        <div class="my-4 flex flex-col md:flex-row gap-4">
            <div class="flex items-center">
                {{ $t('nOutOfTotalChoralesFoundForSerachParams', { n: filteredElements.length, total: chorales.length }) }}
            </div>
        </div>

        <template v-if="filteredElements.length > 0">
            <Subheading>{{ $t('totalDegreeCount') }}</Subheading>
            <DataTable small :items="totalTableItems" :headers="totalTableHeaders">
                <template #[`item.fbFigure`]="{ item }">
                    <div class="text-center font-bold">
                        <NuxtLink v-if="item.fbFigure !== t('total')" :href="localePath({ name: 'bach-phrases', query: { fb: item.fbFigure } })">
                            <code class="text-xs bg-gray-100 rounded p-1">{{ item.fbFigure }}</code>
                        </NuxtLink>
                        <code v-else class="text-xs bg-gray-100 rounded p-1">{{ item.fbFigure }}</code>
                    </div>
                </template>
                <template v-for="i in totalTableHeaders.map(a => a.value).filter(a => a !== 'fbFigure' && a !== 'total')" #[`item.${i}`]="{ item }">
                    <div class="text-center">
                        <template v-if="item[i]">
                            <NuxtLink v-if="item.fbFigure !== t('total')" :href="localePath({ name: 'bach-phrases', query: { degree: i, fb: item.fbFigure } })">
                                {{ item[i] }}
                            </NuxtLink>
                            <template v-else>
                                {{ item[i] }}
                            </template>
                        </template>
                    </div>
                </template>
                <template v-for="i in totalTableHeaders.map(a => a.value).filter(a => a === 'total')" #[`item.${i}`]="{ item }">
                    <div class="text-center">
                        <template v-if="item[i]">
                            {{ item[i] }}
                        </template>
                    </div>
                </template>
                <template v-for="i in totalTableHeaders.map(a => a.value).filter(a => a !== 'fbFigure' && a !== 'total')" #[`head.${i}`]="{ field }">
                    <div class="text-center font-serif">
                        <NuxtLink :href="localePath({ name: 'bach-phrases', query: { degree: field.value } })">
                            {{ field.text }}
                        </NuxtLink>
                    </div>
                </template>
            </DataTable>
            <Hint>
                {{ t('fbFigureInfoText') }}
            </Hint>

            <FiguredBassLegend negative :accid="false" />
            
            <Subheading>{{ $t('degreesOfAllChorales') }}</Subheading>
            <DataTable small :items="tableItems" :headers="tableHeaders">
                <template v-for="i in 23" #[`item.${i}`]="{ item }">
                    <div class="text-center">
                        <template v-if="item[`${i}`]">
                            <button @click="openModal = item[`${i}.phraseId`]; loadScoreData(item[`${i}.filename`])">
                                <span class="font-serif">{{ item[`${i}`] }}</span>
                                <span class="text-gray-300">/</span>
                                <code class="text-xs bg-gray-100 rounded p-1">{{ item[`${i}.fb`] }}</code>
                            </button>
                            <Modal v-if="openModal === item[`${i}.phraseId`]" @close="openModal = null" :title="item[`${i}.phraseId`]">
                                <VerovioCanvas :data="modalScoreData" :scale="35" :page-margin="50" />
                            </Modal>
                        </template>
                        <template v-else>–</template>
                    </div>
                </template>
                <template #[`head.id`]="{ field }">
                    <div class="text-left">
                        {{ field.text }}
                    </div>
                </template>
                <template #[`item.id`]="{ item }">
                    <NuxtLink :href="localePath({ name: 'bach-chorale-nr', params: { nr: item.nr } })">
                        {{ item.title }}
                    </NuxtLink>
                </template>
            </DataTable>
        </template>
    </Container>
</template>
