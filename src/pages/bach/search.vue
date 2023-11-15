<script setup>
definePageMeta({
    layout: 'bach',
});

const { t } = useI18n();
const localePath = useLocalePath();

useHead({
    title: t('bachChorales'),
});

const q = ref();

watch(q, (value) => {
    router.replace({ query: { q: toRaw(value) } });
});

onMounted(() => {
    q.value = route.query.q;
});

const router = useRouter();
const route = useRoute();

const { data: phrasesData } = await useAsyncData('/bach-phrases', () => queryContent('/bach-phrases').find())
const { data: choraleData } = await useAsyncData('/bach-370-chorales', () => queryContent('/bach-370-chorales').find())
const chorales = createBachChorales(choraleData.value, phrasesData.value);
const { filteredElements } = useBachChoraleFilter(chorales);

const qValue = computed(() => {
    return q.value?.replace(/^,+|,+$/g, '').split(/(?=[+\-])\s*|\s*,\s*|\s+/).filter(n => n).join(',');
});

const filteredChorales = computed(() => {
    return filteredElements.value.filter(c => !qValue.value || c.cantusFirmusMint.replace(/[A-Za-z]/g, '').includes(qValue.value) || c.cantusFirmusMint.includes(qValue.value));
});

const { items, addItems } = useArrayLoader(filteredChorales);

function hrefBuilder(chorale) {
    return localePath({ name: 'bach-chorale-nr', params: { nr: chorale.nr } });
}
</script>

<template>
    <Container>

        <Heading>{{ $t('cantusFirmusSearch') }}</Heading>

        <BachChoraleSearchFilter />

        <div>
            <FormInputField v-model="q" :label="$t('mintSearch')" placeholder="+4,-2,-2,-2,-2,+2 or -P5,+P5,+m2" class="text-2xl" />
        </div>

        <div class="my-4 flex flex-col md:flex-row gap-4">
            <div class="flex items-center">
                {{ $t('nOutOfTotalChoralesFoundForSerachParams', { n: filteredChorales.length, total: chorales.length }) }}
            </div>
        </div>

        <InfiniteScroll @load="addItems()" :all="items.length === filteredElements.length">
            <div class="grid grid-cols-1 gap-4">
                <div v-for="chorale in items" :key="`${chorale.id}${qValue}`">
                    <ChoraleListItem :chorale="chorale" :href-builder="hrefBuilder" full-score :highlight-mint="qValue"/>
                </div>
            </div>
        </InfiniteScroll>

    </Container>
</template>
