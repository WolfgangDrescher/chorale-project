<script setup>
const { t } = useI18n();

useHead({
    title: t('kittelChorales'),
});

const { data } = await useAsyncData('/kittel-24-chorales', () => queryContent('/kittel-24-chorales').find())
const chorales = createKittelChorales(data.value);
const { items, addItems } = useArrayLoader(chorales);
</script>

<template>
    <Container>

        <Heading>{{ $t('kittelChorales') }}</Heading>

        <InfiniteScroll @load="addItems()" :all="items.length === chorales.length">
            <div class="grid grid-cols-1 lg:grid-cols-2 gap-4">
                <div v-for="chorale in items" :key="chorales.id">
                    <ChoraleListItem :chorale="chorale" />
                </div>
            </div>
        </InfiniteScroll>

    </Container>
</template>
