<script setup>
definePageMeta({
    layout: 'bach',
});

const { t } = useI18n();
const localePath = useLocalePath();

useHead({
    title: t('bachChorales'),
});

const { data: phrasesData } = await useAsyncData('/bach-phrases', () => queryContent('/bach-phrases').find())
const { data: choraleData } = await useAsyncData('/bach-370-chorales', () => queryContent('/bach-370-chorales').find())
const chorales = createBachChorales(choraleData.value, phrasesData.value);
const { filteredElements } = useBachChoraleFilter(chorales);
const { items, addItems } = useArrayLoader(filteredElements);

function hrefBuilder(chorale) {
    return localePath({name: 'bach-chorale-nr', params: { nr: chorale.nr }});
}
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

        <InfiniteScroll @load="addItems()" :all="items.length === filteredElements.length">
            <div class="grid grid-cols-1 lg:grid-cols-2 gap-4">
                <div v-for="chorale in items" :key="chorale.id">
                    <ChoraleListItem :chorale="chorale" :href-builder="hrefBuilder" />
                </div>
            </div>
        </InfiniteScroll>

    </Container>
</template>
