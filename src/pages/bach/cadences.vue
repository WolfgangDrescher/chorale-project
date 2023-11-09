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
    const maxCadenceItem = filteredElements.value.reduce((prev, current) => {
        return (prev && prev.countCadences > current.countCadences) ? prev : current;
    });
    return maxCadenceItem?.countCadences ?? 0;
});

const tableHeaders = computed(() => {
    return [{
        text: '#',
        value: 'id',
    }, ...Array.from({ length: maxCadences.value }, (_, i) => i + 1).map((n) => ({ text: n, value: n }))];
});

function romanizeDeg(deg) {
    return `${deg.replaceAll(/\d/g, '').replace('-', '♭').replace('+', '♯')}${romanize(deg.replaceAll(/\D/g, ''))}`
}

const tableItems = computed(() => {
    return filteredElements.value.map(chorale => {
        const result = {
            id: chorale.id,
        };
        chorale.cadences.forEach((cadence, index) => {
            result[index + 1] = romanizeDeg(cadence.degree);
        });
        return result;

    });
});

const totalTableHeaders = Array.from({ length: 7 }, (_, i) => i + 1).map((n) => ({ text: romanizeDeg(n), value: n }));
const totalTableItems = computed(() => {
    const items = Array.from({ length: 7 }, (_, i) => i + 1).map((n) => {
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

        <Subheading>{{ $t('totalDegreeCount') }}</Subheading>
        <DataTable small :items="totalTableItems" :headers="totalTableHeaders" />
        
        <Subheading>{{ $t('degreesOfAllChorales') }}</Subheading>
        <DataTable small :items="tableItems" :headers="tableHeaders" />
    </Container>
</template>
