<script setup>
const { t } = useI18n();
const localePath = useLocalePath();

useHead({
    title: t('schiorringChoralBog'),
});

const { data } = await useAsyncData('/schiorring-choral-bog', () => queryContent('/schiorring-choral-bog').find())
const chorales = createSchiorringChorales(data.value);
const { items, addItems } = useArrayLoader(chorales);

function hrefBuilder(chorale) {
    return localePath({ name: 'schiorring-nr', params: { nr: chorale.nr } });
}
</script>

<template>
    <Container>

        <Heading>{{ $t('schiorringChoralBog') }}</Heading>

        <InfiniteScroll @load="addItems()" :all="items.length === chorales.length">
            <div class="grid grid-cols-1 lg:grid-cols-2 gap-4">
                <div v-for="chorale in items" :key="chorales.id">
                    <ChoraleListItem :chorale="chorale" :href-builder="hrefBuilder" />
                </div>
            </div>
        </InfiniteScroll>

    </Container>
</template>
