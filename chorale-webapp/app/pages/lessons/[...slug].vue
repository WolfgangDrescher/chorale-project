<script setup>
const { t } = useI18n();
</script>

<template>
    <ContentCollectionPage collection="lessons" :navigation-fields="['lessonNumber']">
        <template #links="{ page }">
            <UButton v-if="page.analyses?.length" :label="t('lessonData')" icon="lucide:chart-bar-big" :to="`${page.path}/data`" color="neutral" variant="outline" />
        </template>
    </ContentCollectionPage>
</template>
