<script setup>
definePageMeta({
    layout: 'bach',
});

const { data: phrasesData } = await useAsyncData('/bach-phrases', () => queryContent('/bach-phrases').find())
const { data: choraleData } = await useAsyncData('/bach-370-chorales', () => queryContent('/bach-370-chorales').find())
const chorales = createBachChorales(choraleData.value, phrasesData.value);
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

const phrases = computed(() => {
    return filteredElements.value.map(c => c.phrases).flat().filter(phrase => {
        const filterPhraseDegrees = (phraseDegrees) => {
            if (!phraseDegrees || !phraseDegrees.length) return true;
            return phraseDegrees.includes(phrase.degree);
        };
        const filterPhraseFbNumbers = (phraseFbNumbers) => {
            if (!phraseFbNumbers || !phraseFbNumbers.length) return true;
            return phraseFbNumbers.includes(phrase.fb);
        };
        return filterPhraseDegrees(filter.degree) && filterPhraseFbNumbers(filter.fb)
    });
});

const { items, addItems } = useArrayLoader(phrases);

const totalPhrases = computed(() => {
    return chorales.reduce((accumulator, chorale) => accumulator + chorale.phrases.length, 0);
});

const cadenceDegreeOptions = [...new Set(chorales.map(chorale => chorale.phrases.map(c => c.degree)).flat().filter(n => n))].sort(sortphraseDegrees).map(degree => ({
    value: degree,
    text: romanizeDeg(degree),
}));

const cadenceDegreeFbNumberOptions = [...new Set(chorales.map(chorale => chorale.phrases.map(c => c.fb)).flat().filter(n => n))].sort().map(fb => ({
    value: fb,
    text: fb,
}));
</script>

<template>
    <Container>

        <Heading>{{ $t('choralePhrases') }}</Heading>

        <div class="grid grid-cols-4 gap-4 mb-4">
            <div>
                <FormDropdown v-model="filter.degree" :label="$t('phraseDegrees')" :options="cadenceDegreeOptions" :search-enabled="false" :multiple="true" />
            </div>
            <div>
                <FormDropdown v-model="filter.fb" :label="$t('phraseFbNumbers')" :options="cadenceDegreeFbNumberOptions" :search-enabled="false" :multiple="true" />
            </div>
        </div>

        <div class="my-4 flex flex-col md:flex-row gap-4">
            <div class="flex items-center">
                {{ $t('nOutOfTotalPhrasesFoundForSerachParams', { n: phrases.length, total: totalPhrases }) }}
            </div>
        </div>

        <InfiniteScroll @load="addItems()" :all="items.length === phrases.length">
            <div class="grid grid-cols-1 lg:grid-cols-2 gap-4">
                <div v-for="phrase in items" :key="phrase.id">
                    <PhraseListItem :phrase="phrase" />
                </div>
            </div>
        </InfiniteScroll>

    </Container>
</template>
