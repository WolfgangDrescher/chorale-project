<script setup>
const { t } = useI18n();

useHead({
    title: t('bachChorales'),
});

const { data } = await useAsyncData('/bach-370-chorales', () => queryContent('/bach-370-chorales').find())
const chorales = createBachChorales(data.value);
const { items, addItems } = useArrayLoader(chorales);
</script>

<template>
    <Container>

        <Heading>{{ $t('bachChorales') }}</Heading>

        <InfiniteScroll @load="addItems()" :all="items.length === chorales.length">
            <div class="grid grid-cols-1 lg:grid-cols-2 gap-4">
                <div v-for="chorale in items" :key="chorales.id">
                    <ChoraleListItem :chorale="chorale" />
                </div>
            </div>
        </InfiniteScroll>

    </Container>
</template>
