<script setup>
import { kebabCase } from 'scule';

const { t } = useI18n();
const route = useRoute();
const path = `/lessons/${route.params.slug}`;

const { data: lesson } = await useAsyncData(`lesson-data-${kebabCase(path)}`, () => {
    return queryCollection('lessons').path(path).select('title', 'analyses').first();
});

if (!lesson.value?.analyses?.length) {
    throw createError({ statusCode: 404, statusMessage: 'Page not found', fatal: true });
}
</script>

<template>
    <ContentCollectionPage collection="lessons" :navigation-fields="['lessonNumber']" :path="path" :title="`${lesson.title}: ${t('lessonData')}`">
        <template #links>
            <UButton :label="t('lessonBackToText')" icon="lucide:arrow-left" :to="path" color="neutral" variant="outline" />
        </template>
        <template #default="{ page }">
            <LessonData :lesson="page" />
        </template>
    </ContentCollectionPage>
</template>
