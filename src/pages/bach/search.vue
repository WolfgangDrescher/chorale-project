<script setup>
definePageMeta({
    layout: 'bach',
});

const { t } = useI18n();
const localePath = useLocalePath();

useHead({
    title: t('bachChorales'),
});

const filter = reactive({
    q: '',
    ignoreFermatas: true,
});

watch(filter, (value) => {
    router.replace({ query: value });
});

onMounted(() => {
    filter.q = route.query.q;
});

const router = useRouter();
const route = useRoute();

const { data: phrasesData } = await useAsyncData('/bach-phrases', () => queryContent('/bach-phrases').find())
const { data: choraleData } = await useAsyncData('/bach-370-chorales', () => queryContent('/bach-370-chorales').find())
const chorales = createBachChorales(choraleData.value, phrasesData.value);
const { filteredElements } = useBachChoraleFilter(chorales);

const qValue = computed(() => {
    return filter.q?.replace(/^[,;]+/g, '').replaceAll(/\s*,\s+|\s+/g, ',').replace(';', filter.ignoreFermatas ? ',' : ';');
});

const filteredChorales = computed(() => {
    return filteredElements.value.filter(c => {
        const mintNoQuality = c.cantusFirmusMint.replace(/[AdmMP]/g, '').replace(';', filter.ignoreFermatas ? ',' : ';');
        const mint = c.cantusFirmusMint.replace(';', filter.ignoreFermatas ? ',' : ';');
        const q = qValue.value;
        return !q ||
        mintNoQuality.includes(q) ||
        mint.includes(q);
    });
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

        <div class="grid grid-cols-2 gap-4">
            <div>
                <FormInputField v-model="filter.q" :label="$t('mintSearch')" placeholder="+4,-2,-2,-2,-2,+2 or -P5,+P5,+m2" />
            </div>
            <div>
                <FormCheckbox v-model="filter.ignoreFermatas" :label="$t('ignoreFermatas')" group-label="" />
            </div>
        </div>

        <div class="my-4 flex flex-col md:flex-row gap-4">
            <div class="flex items-center">
                {{ $t('nOutOfTotalChoralesFoundForSerachParams', { n: filteredChorales.length, total: chorales.length }) }}
            </div>
        </div>

        <InfiniteScroll @load="addItems()" :all="items.length === filteredElements.length">
            <div class="grid grid-cols-1 gap-4">
                <div v-for="chorale in items" :key="`${chorale.id}${qValue}`">
                    <ChoraleListItem :chorale="chorale" :href-builder="hrefBuilder" full-score :highlight-mint="qValue" :highlight-ignore-fermatas="filter.ignoreFermatas"/>
                </div>
            </div>
        </InfiniteScroll>

    </Container>
</template>
