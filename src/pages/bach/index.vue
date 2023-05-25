<script setup>
const { t } = useI18n();
const localePath = useLocalePath();

useHead({
    title: t('bachChorales'),
});

const { data } = await useAsyncData('/bach-370-chorales', () => queryContent('/bach-370-chorales').find())
const chorales = createBachChorales(data.value);
const { filteredElements } = useBachChoraleFilter(chorales);
const { items, addItems } = useArrayLoader(filteredElements);

function hrefBuilder(chorale) {
    return localePath({name: 'bach-nr', params: { nr: chorale.nr }});
}
</script>

<template>
    <Container>

        <Heading>{{ $t('bachChorales') }}</Heading>

        <BachChoraleSearchFilter />

        <InfiniteScroll @load="addItems()" :all="items.length === filteredElements.length">
            <div class="grid grid-cols-1 lg:grid-cols-2 gap-4">
                <div v-for="chorale in items" :key="chorale.id">
                    <ChoraleListItem :chorale="chorale" :href-builder="hrefBuilder" />
                </div>
            </div>
        </InfiniteScroll>

    </Container>
</template>
