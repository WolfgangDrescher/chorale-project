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
    mint: '',
    hint: '',
    ignoreFermatas: true,
});

watch(filter, (value) => {
    router.replace({
        query: {
            mint: value.mint || undefined,
            hint: value.hint || undefined,
        },
    });
});

onMounted(() => {
    filter.mint = route.query.mint;
    filter.hint = route.query.hint;
});

const router = useRouter();
const route = useRoute();

const { data: choraleData } = await useAsyncData('/bach-370-chorales', () => queryContent('/bach-370-chorales').find())
const chorales = createBachChorales(choraleData.value);
const { filteredElements } = useBachChoraleFilter(chorales);

const mintValue = computed(() => {
    return filter.mint?.replace(/^[,;]+/g, '').replaceAll(/\s*,\s+|\s+/g, ',').replace(';', filter.ignoreFermatas ? ',' : ';');
});

const hintValue = computed(() => {
    return filter.hint?.replace(/^[,;]+/g, '').replace(';', filter.ignoreFermatas ? ',' : ';');
});

const filteredChorales = computed(() => {
    return filteredElements.value.filter(c => {
        const mintNoQuality = c.cantusFirmusMint.replace(/[AdmMP]/g, '').replace(';', filter.ignoreFermatas ? ',' : ';');
        const mintString = c.cantusFirmusMint.replace(';', filter.ignoreFermatas ? ',' : ';');
        const mint = mintValue.value;
        const hintNoQuality = c.harmonicIntervals.replace(/[AdmMP]/g, '').replace(';', filter.ignoreFermatas ? ',' : ';');
        const hintString = c.harmonicIntervals.replace(';', filter.ignoreFermatas ? ',' : ';');
        const hint = hintValue.value;
        return (
            !mint ||
            mintNoQuality.includes(mint) ||
            mintString.includes(mint)
        ) && (
            !hint ||
            hintNoQuality.includes(hint) ||
            hintString.includes(hint)
        );
    });
});

const { items, addItems } = useArrayLoader(filteredChorales);

function hrefBuilder(chorale) {
    return localePath({ name: 'bach-chorale-nr', params: { nr: chorale.nr } });
}
</script>

<template>
    <Container>

        <Heading>{{ $t('search') }}</Heading>

        <BachChoraleSearchFilter />

        <div class="grid grid-cols-2 gap-4">
            <div>
                <FormInputField v-model="filter.mint" :label="$t('mintCantusFirmusSearch')" placeholder="+4,-2,-2,-2,-2,+2 or -P5,+P5,+m2" />
            </div>
                <div>
                    <FormInputField v-model="filter.hint" :label="$t('hintSearch')" placeholder="6 4 3 or M6 A4 M2" />
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
                <div v-for="chorale in items" :key="`${chorale.id}/${filteredElements.map(e => e.id)}}`">
                    <ChoraleListItem
                        :chorale="chorale"
                        :href-builder="hrefBuilder"
                        full-score
                        :highlight-mint="mintValue"
                        :highlight-hint="hintValue"
                        :highlight-ignore-fermatas="filter.ignoreFermatas"
                    />
                </div>
            </div>
        </InfiniteScroll>

    </Container>
</template>
