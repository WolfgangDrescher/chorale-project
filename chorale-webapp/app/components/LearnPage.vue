<script setup>
const props = defineProps({
    collection: { type: String, required: true },
    path: { type: String, default: null },
    breadcrumb: { type: Array, default: () => [] },
});

const { t, d } = useI18n();
const route = useRoute();
const path = props.path ?? route.path;
const slug = slugOfPath(path);

const { data: page } = await useAsyncData(`${props.collection}-${slug}`, () => {
    return queryCollection(props.collection).path(path).first();
});

if (!page.value) {
    throw createError({ statusCode: 404, statusMessage: 'Page not found', fatal: true });
}

const { data: backlinks } = await useLearnBacklinks(LEARN_COLLECTION_PAGES[props.collection].reference, slug);

const { data: surround } = await useAsyncData(`${props.collection}-${slug}-surround`, async () => {
    const pages = await queryCollection(props.collection).order('title', 'ASC').select('path', 'title').all();
    const index = pages.findIndex((entry) => entry.path === path);
    const short = (entry) => entry && { path: entry.path, title: entry.title };
    return [short(pages[index - 1]), short(pages[index + 1])];
});

const queryDrawerOpen = ref(false);
const activeQueryId = ref(null);
const activeQuery = computed(() => page.value.queries?.find((query) => query.id === activeQueryId.value)?.query.trimEnd() ?? '');

function openQuery(id) {
    activeQueryId.value = id;
    queryDrawerOpen.value = true;
}

provide('lesson', page.value);
provide('openQuery', openQuery);

const ANSWER_CLASSES = { yes: 'border-green-500 bg-green-500/8', no: 'border-red-500 bg-red-500/8' };
const VERDICT_TEXT = { yes: 'text-green-700', no: 'text-red-700' };

const tagsTo = `/${props.collection}`;

const metaItems = computed(() => [
    page.value.date && { icon: 'lucide:calendar', text: d(new Date(page.value.date), LEARN_DATE_FORMAT) },
    lastChangeItem(page.value, t, d),
].filter(Boolean));

useSeoMeta({
    title: page.value.title,
    description: page.value.answer,
});
</script>

<template>
    <UContainer>
        <UPage :ui="{ root: 'lg:grid-cols-[280px_minmax(0,1fr)]', left: 'lg:col-span-1', center: 'lg:col-span-1' }">
            <template #left>
                <UPageAside>
                    <LearnNavigation />
                </UPageAside>
            </template>

            <UPageHeader :title="page.title" :ui="{ title: 'text-2xl sm:text-3xl' }">
                <template #headline>
                    <UBreadcrumb :items="breadcrumb" :ui="{ linkLabel: 'max-w-64 truncate' }" />
                </template>
                <PageMetaLine :items="metaItems" />
                <p v-if="page.tags?.length" class="mt-3 flex flex-wrap items-center gap-1.5">
                    <UBadge v-for="tag in page.tags" :key="tag" :label="tag" :to="{ path: tagsTo, query: { tag } }" as="NuxtLink" color="neutral" variant="outline" class="hover:bg-elevated" />
                </p>
            </UPageHeader>

            <UPageBody>
                <div class="rounded-r-lg border-l-4 px-5 py-4" :class="ANSWER_CLASSES[page.verdict] ?? 'border-accented bg-elevated/50'">
                    <p v-if="page.verdict" class="mb-1 text-xl font-bold leading-tight" :class="VERDICT_TEXT[page.verdict]">{{ t(`faqVerdict.${page.verdict}`) }}</p>
                    <p class="text-lg font-medium text-highlighted text-pretty">{{ page.answer }}</p>
                    <UButton
                        v-if="page.stat"
                        :icon="page.stat.query ? 'lucide:search' : undefined"
                        :trailing-icon="page.stat.query ? 'lucide:arrow-right' : undefined"
                        :label="[page.stat.value, page.stat.label, page.stat.chorales && t('learnStatChorales', Number(page.stat.chorales))].filter(Boolean).join(' ')"
                        :disabled="!page.stat.query"
                        :title="page.stat.query ? t('showQueryResults') : undefined"
                        color="neutral"
                        variant="outline"
                        size="sm"
                        class="mt-3 disabled:opacity-100"
                        @click="openQuery(page.stat.query)"
                    />
                </div>

                <ContentRenderer :value="page" />

                <section v-if="page.queries?.length">
                    <h2 class="mb-3 text-lg font-semibold text-highlighted">{{ t('learnQueries') }}</h2>
                    <div class="-mx-0.5 flex flex-wrap gap-2">
                        <QueryRef v-for="query in page.queries" :id="query.id" :key="query.id" :label="query.title" />
                    </div>
                </section>

                <USeparator />

                <section v-if="backlinks?.lessons.length">
                    <h2 class="mb-3 text-lg font-semibold text-highlighted">{{ t('learnUsedIn') }}</h2>
                    <div class="flex flex-wrap gap-2">
                        <UButton
                            v-for="lesson in backlinks.lessons"
                            :key="lesson.path"
                            :to="lesson.path"
                            :label="`${t('lessonNumber', { number: lesson.lessonNumber })}: ${lesson.title}`"
                            :icon="LEARN_PARTS.lessons.icon"
                            :ui="{ leadingIcon: LEARN_PARTS.lessons.color }"
                            color="neutral"
                            variant="outline"
                            size="sm"
                        />
                    </div>
                </section>
                <template v-if="surround?.some(Boolean)">
                    <USeparator />
                    <UContentSurround :surround="surround" />
                </template>
            </UPageBody>
        </UPage>
        <QueryDrawer v-if="page.queries?.length" v-model:open="queryDrawerOpen" :query="activeQuery" sort-by="density" />
    </UContainer>
</template>
