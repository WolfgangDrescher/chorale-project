<script setup>
import { kebabCase } from 'scule';
import { findPageHeadline } from '@nuxt/content/utils';

const route = useRoute();

const { data: page } = await useAsyncData(`docs-${kebabCase(route.path)}`, () => {
    return queryCollection('docs').path(route.path).first();
});

const { data: surround } = await useAsyncData(`docs-${kebabCase(route.path)}-surround`, () => {
    return queryCollectionItemSurroundings('docs', route.path, {
      fields: ['description'],
    });
});

if (!page.value) {
    throw createError({ statusCode: 404, statusMessage: 'Page not found', fatal: true });
}

const { data: navigation } = await useAsyncData('docs-navigation', () => {
    return queryCollectionNavigation('docs');
});

const headline = ref(findPageHeadline(navigation?.value, page.value?.path))

useSeoMeta({
    title: page.value.title,
    description: page.value.description,
});
</script>

<template>
    <UContainer>
        <UPage>
            <template #left>
                <UPageAside>
                    <UContentNavigation :navigation="navigation ?? []" highlight />
                </UPageAside>
            </template>

            <UPageHeader :title="page.title" :description="page.description" :headline="headline" />

            <UPageBody>
                <ContentRenderer v-if="page" :value="page" />
                <USeparator />
                <UContentSurround :surround="surround" />
            </UPageBody>
        </UPage>
    </UContainer>
</template>
