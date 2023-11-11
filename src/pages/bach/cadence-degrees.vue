<script setup>
// import { gray } from 'tailwindcss/colors';
const gray50 = '#f9fafb'; // gray[50] will not work with nuxt generate

const { t } = useI18n();
const localePath = useLocalePath();

useHead({
    title: t('cadenceDegrees'),
});

const { data: cadenceData } = await useAsyncData('/bach-cadences', () => queryContent('/bach-cadences').find())
const { data: choraleData } = await useAsyncData('/bach-370-chorales', () => queryContent('/bach-370-chorales').find())
const chorales = createBachChorales(choraleData.value, cadenceData.value);
const { filteredElements } = useBachChoraleFilter(chorales);


const maxCadences = computed(() => {
    if (filteredElements.value.length === 1) {
        return filteredElements.value[0].countCadences
    } else if (filteredElements.value.length >= 2) {
        const maxCadenceItem = filteredElements.value?.reduce((prev, current) => {
            return (prev && prev.countCadences > current.countCadences) ? prev : current;
        });
        return maxCadenceItem?.countCadences;
    }
    return 0;
});

const tableHeaders = computed(() => {
    return [{
        text: '#',
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
        chorale.cadences.forEach((cadence, index) => {
            result[index + 1] = romanizeDeg(cadence.degree);
        });
        chorale.cadences.forEach((cadence, index) => {
            result[`${index + 1}.fb`] = cadence.fb;
        });
        return result;

    });
});

const totalTableHeaders = computed(() => {
    const degreeItems = filteredElements.value.reduce((accumulator, chorale) => {
        chorale.cadences.forEach(cadence => {
            if (!accumulator.includes(cadence.degree)) {
                accumulator.push(cadence.degree);
            }
        });
        return accumulator;
    }, []).sort(sortCadenceDegrees).map(n => ({ text: romanizeDeg(n), value: n, align: 'center' }))
    return [{ text: t('fbFigures'), value: 'fbFigures', align: 'center' }, ...degreeItems, { text: t('total'), value: 'total', align: 'center', cellBgColor: gray50 }];
});
const totalTableItems = computed(() => {
    const result = [];

    // Count cadences for each fb figure and for each degree
    const fbFigures = [...new Set(filteredElements.value.map(chorale => chorale.cadences.map(c => c.fb)).flat())].sort();
    fbFigures.forEach((fbFig) => {
        const items = totalTableHeaders.value.map(({ value: n }) => {
            const count = filteredElements.value.reduce((accumulator, chorale) => {
                const num = chorale.cadences.filter(cadence => cadence.degree === n && fbFig === cadence.fb).length;
                return accumulator + num;
            }, 0);
            return [n, count];
        });
        const fbFigObj = Object.fromEntries(items)
        fbFigObj.fbFigures = fbFig;
        fbFigObj.total = items.reduce((accumulator, [, a]) => accumulator + a, 0)
        result.push(fbFigObj);
    });

    // Count all cadences for each degree
    const items = totalTableHeaders.value.map(({ value: n }) => {
        const count = filteredElements.value.reduce((accumulator, chorale) => {
            const num = chorale.cadences.filter(cadence => cadence.degree === n).length;
            return accumulator + num;
        }, 0);
        return [n, count];
    });
    const totalObj = Object.fromEntries(items)
    totalObj.fbFigures = t('total');
    totalObj.total = items.reduce((accumulator, [,a]) => accumulator + a, 0)
    totalObj._rowBgColor = gray50;
    result.push(totalObj)

    return result;
});
</script>

<template>
    <Container>

        <Heading>{{ $t('cadenceDegrees') }}</Heading>

        <BachChoraleSearchFilter />

        <div class="my-4 flex flex-col md:flex-row gap-4">
            <div class="flex items-center">
                {{ $t('nOutOfTotalChoralesFoundForSerachParams', { n: filteredElements.length, total: chorales.length }) }}
            </div>
        </div>

        <template v-if="filteredElements.length > 0">
            <Subheading>{{ $t('totalDegreeCount') }}</Subheading>
            <DataTable small :items="totalTableItems" :headers="totalTableHeaders">
                <template #[`item.fbFigures`]="{ item }">
                    <div class="text-center font-bold">
                        <code class="text-xs bg-gray-100 rounded p-1">{{ item.fbFigures }}</code>
                    </div>
                </template>
                <template v-for="i in totalTableHeaders.map(a => a.value).filter(a => a !== 'fbFigures')" #[`item.${i}`]="{ item }">
                    <div class="text-center">
                        <template v-if="item[i]">
                            {{ item[i] }}
                        </template>
                    </div>
                </template>
                <template v-for="i in totalTableHeaders.map(a => a.value).filter(a => a !== 'fbFigures' && a !== 'total')" #[`head.${i}`]="{ field }">
                    <div class="text-center font-serif">
                        {{ field.text }}
                    </div>
                </template>
            </DataTable>
            
            <Subheading>{{ $t('degreesOfAllChorales') }}</Subheading>
            <DataTable small :items="tableItems" :headers="tableHeaders">
                <template v-for="i in 23" #[`item.${i}`]="{ item }">
                    <div class="text-center">
                        <template v-if="item[`${i}`]">
                            <span class="font-serif">{{ item[`${i}`] }}</span>
                            <span class="text-gray-300">/</span>
                            <code class="text-xs bg-gray-100 rounded p-1">{{ item[`${i}.fb`] }}</code>
                        </template>
                        <template v-else>–</template>
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
