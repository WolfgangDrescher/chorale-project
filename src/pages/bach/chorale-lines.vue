<script setup>
const { data: cadenceData } = await useAsyncData('/bach-cadences', () => queryContent('/bach-cadences').find())
const { data: choraleData } = await useAsyncData('/bach-370-chorales', () => queryContent('/bach-370-chorales').find())
const chorales = createBachChorales(choraleData.value, cadenceData.value);
const { filteredElements } = useBachChoraleFilter(chorales);

const router = useRouter();
const route = useRoute();

const filter = reactive({
    degree: [],
    fb: [],
});

watch(filter, (value) => {
    router.replace({query: toRaw(value)});
});

onMounted(() => {
    Object.entries(route.query).forEach(([key, value]) => {
        if (['degree', 'fb'].includes(key)) {
            filter[key] = [value].flat();
        } else {
            filter[key] = value;
        }
    });
});

const choraleLines = computed(() => {
    return filteredElements.value.map(c => c.cadences).flat().filter(choraleLine => {
        const filterCadenceDegrees = (cadenceDegrees) => {
            if (!cadenceDegrees || !cadenceDegrees.length) return true;
            return cadenceDegrees.includes(choraleLine.degree);
        };
        const filterCadenceDegreeFbNumbers = (cadenceDegreeFbNumbers) => {
            if (!cadenceDegreeFbNumbers || !cadenceDegreeFbNumbers.length) return true;
            return cadenceDegreeFbNumbers.includes(choraleLine.fb);
        };
        return filterCadenceDegrees(filter.degree) && filterCadenceDegreeFbNumbers(filter.fb)
    });
});

const { items, addItems } = useArrayLoader(choraleLines);

const totalChoraleLines = computed(() => {
    return chorales.reduce((accumulator, chorale) => accumulator + chorale.cadences.length, 0);
});

const cadenceDegreeOptions = [...new Set(chorales.map(chorale => chorale.cadences.map(c => c.degree)).flat().filter(n => n))].sort(sortCadenceDegrees).map(degree => ({
    value: degree,
    text: romanizeDeg(degree),
}));

const cadenceDegreeFbNumberOptions = [...new Set(chorales.map(chorale => chorale.cadences.map(c => c.fb)).flat().filter(n => n))].sort().map(fb => ({
    value: fb,
    text: fb,
}));
</script>

<template>
    <Container>

        <Heading>{{ $t('choralLines') }}</Heading>

        <div class="grid grid-cols-4 gap-4 mb-4">
            <div>
                <FormDropdown v-model="filter.degree" :label="$t('cadenceDegrees')" :options="cadenceDegreeOptions" :search-enabled="false" :multiple="true" />
            </div>
            <div>
                <FormDropdown v-model="filter.fb" :label="$t('cadenceDegreeFbNumbers')" :options="cadenceDegreeFbNumberOptions" :search-enabled="false" :multiple="true" />
            </div>
        </div>

        <div class="my-4 flex flex-col md:flex-row gap-4">
            <div class="flex items-center">
                {{ $t('nOutOfTotalChoraleLinesFoundForSerachParams', { n: choraleLines.length, total: totalChoraleLines }) }}
            </div>
        </div>

        <InfiniteScroll @load="addItems()" :all="items.length === choraleLines.length">
            <div class="grid grid-cols-1 lg:grid-cols-2 gap-4">
                <div v-for="choraleLine in items" :key="choraleLine.id">
                    <ChoraleLineListItem :chorale-line="choraleLine" />
                </div>
            </div>
        </InfiniteScroll>

    </Container>
</template>
