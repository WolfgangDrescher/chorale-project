<script setup>
const { t } = useI18n();
const localePath = useLocalePath();

useHead({
    title: t('bachChorales'),
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
    }, ...Array.from({ length: maxCadences.value }, (_, i) => i + 1).map((n) => ({ text: n, value: n, align: 'center' }))];
});

function romanizeDeg(deg) {
    return `${deg.replaceAll(/\d/g, '').replace('-', '♭').replace('+', '♯')}${romanize(deg.replaceAll(/\D/g, ''))}`
}

const tableItems = computed(() => {
    return filteredElements.value.map(chorale => {
        const result = {
            id: chorale.id,
            nr: chorale.nr,
        };
        chorale.cadences.forEach((cadence, index) => {
            result[index + 1] = romanizeDeg(cadence.degree);
        });
        return result;

    });
});

const totalTableHeaders = computed(() => {
    return filteredElements.value.reduce((accumulator, chorale) => {
        chorale.cadences.forEach(cadence => {
            if (!accumulator.includes(cadence.degree)) {
                accumulator.push(cadence.degree);
            }
        });
        return accumulator;
    }, []).sort((a, b) => {
        if (parseInt(a.replaceAll(/\D/g, ''), 10) > parseInt(b.replaceAll(/\D/g, ''), 10)) return 1;
        if (parseInt(a.replaceAll(/\D/g, ''), 10) < parseInt(b.replaceAll(/\D/g, ''), 10)) return -1;
        if (a.includes('+') && !b.includes('+')) return 1;
        if (a.includes('-') && !b.includes('-')) return -1;
    }).map(n => ({text: romanizeDeg(n), value: n, align: 'center' }));
});
const totalTableItems = computed(() => {
    const items = totalTableHeaders.value.map(({value: n}) => {
        const count = filteredElements.value.reduce((accumulator, chorale) => {
            const num = chorale.cadences.filter(cadence => cadence.degree === n).length;
            return accumulator + num;
        }, 0);
        return [n, count];
    });
    return [Object.fromEntries(items)];
});
</script>

<template>
    <Container>

        <Heading>{{ $t('bachChorales') }}</Heading>

        <BachChoraleSearchFilter />

        <div class="my-4 flex flex-col md:flex-row gap-4">
            <div class="flex items-center">
                {{ $t('nOutOfTotalChoralesFoundForSerachParams', { n: filteredElements.length, total: chorales.length }) }}
            </div>
        </div>

        <template v-if="filteredElements.length > 0">
            <Subheading>{{ $t('totalDegreeCount') }}</Subheading>
            <DataTable small :items="totalTableItems" :headers="totalTableHeaders">
                <template v-for="i in totalTableHeaders.map(a => a.value)" #[`item.${i}`]="{ item }">
                    <div class="text-center">
                        {{ item[`${i}`] }}
                    </div>
                </template>
            </DataTable>
            
            <Subheading>{{ $t('degreesOfAllChorales') }}</Subheading>
            <DataTable small :items="tableItems" :headers="tableHeaders">
                <template v-for="i in 23" #[`item.${i}`]="{ item }">
                    <div class="text-center">
                        {{ item[`${i}`] }}
                    </div>
                </template>
                <template #[`item.id`]="{ item }">
                    <div class="text-center">
                        <NuxtLink :href="localePath({ name: 'bach-nr', params: { nr: item.nr } })">
                            {{ item.id }}
                        </NuxtLink>
                    </div>
                </template>
            </DataTable>
        </template>
    </Container>
</template>
