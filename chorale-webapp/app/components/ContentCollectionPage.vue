<script setup>
import { kebabCase } from 'scule';
import { findPageHeadline } from '@nuxt/content/utils';

const props = defineProps({
    collection: { type: String, required: true },
    // Fields of the front matter every entry of the navigation gets besides its title and path:
    // lessonNumber, for the number in front of a lesson.
    navigationFields: { type: Array, default: () => [] },
    // The page shown, when it is not the one of the route: the data of a lesson is a route below it.
    path: { type: String, default: null },
    // The title of the browser tab, when it is not that of the page.
    title: { type: String, default: null },
});

const { t } = useI18n();
const route = useRoute();
const path = props.path ?? route.path;
const key = `${props.collection}-${kebabCase(path)}`;

const { data: page } = await useAsyncData(key, () => {
    return queryCollection(props.collection).path(path).first();
});

const { data: surround } = await useAsyncData(`${key}-surround`, () => {
    return queryCollectionItemSurroundings(props.collection, path, {
        fields: ['description'],
    });
});

if (!page.value) {
    throw createError({ statusCode: 404, statusMessage: 'Page not found', fatal: true });
}

const { data: navigationData } = await useAsyncData(`${props.collection}-navigation`, () => {
    return queryCollectionNavigation(props.collection, props.navigationFields);
});

// A lesson is listed with its number in front of its title, and stays marked on a route below it.
function withExactMatch(items) {
    return (items ?? []).map((item) => ({
        ...item,
        title: item.lessonNumber ? `${item.lessonNumber}. ${item.title}` : item.title,
        exact: true,
        ...(item.path === path ? { active: true } : {}),
        children: item.children ? withExactMatch(item.children) : item.children,
    }));
}

const navigation = computed(() => withExactMatch(navigationData.value?.[0]?.children));

const headline = ref(page.value.lessonNumber ? t('lessonNumber', { number: page.value.lessonNumber }) : findPageHeadline(navigation.value, page.value.path));

// The queries a lesson lists in its front matter run in one drawer for the whole page, which the
// references in the text open.
const queryDrawerOpen = ref(false);
const activeQueryId = ref(null);
const activeQuery = computed(() => page.value.queries?.find((query) => query.id === activeQueryId.value)?.query.trimEnd() ?? '');

provide('lesson', page.value);
provide('openQuery', (id) => {
    activeQueryId.value = id;
    queryDrawerOpen.value = true;
});

useSeoMeta({
    title: props.title ?? page.value.title,
    description: page.value.description,
});
</script>

<template>
    <UContainer>
        <UPage :ui="{ root: 'lg:grid-cols-[280px_minmax(0,1fr)]', left: 'lg:col-span-1', center: 'lg:col-span-1' }">
            <template #left>
                <UPageAside>
                    <UContentNavigation :navigation="navigation" highlight />
                </UPageAside>
            </template>

            <UPageHeader :title="page.title" :description="page.description" :headline="headline">
                <template v-if="$slots.links" #links>
                    <slot name="links" :page="page" />
                </template>
            </UPageHeader>

            <UPageBody>
                <slot :page="page">
                    <ContentRenderer :value="page" />
                </slot>
                <USeparator />
                <UContentSurround :surround="surround" />
            </UPageBody>
        </UPage>
        <QueryDrawer v-if="page.queries?.length" v-model:open="queryDrawerOpen" :query="activeQuery" sort-by="density" />
    </UContainer>
</template>
