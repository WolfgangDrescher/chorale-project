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

const isFaq = props.collection === 'faq';

const { data: backlinks } = await useLearnBacklinks(LEARN_COLLECTION_PAGES[props.collection].reference, slug);

// Only the questions name their topics (`topics` in the front matter).
const { data: allRelated } = await useAsyncData(`${props.collection}-${slug}-related`, async () => {
    if (isFaq) {
        const paths = (page.value.topics ?? []).map((topic) => `/topics/${topic}`);
        if (!paths.length) return [];
        return queryCollection('topics').where('path', 'IN', paths).select('path', 'title', 'description').all();
    }
    const questions = await queryCollection('faq').select('path', 'title', 'answer', 'topics').all();
    return questions.filter((question) => question.topics?.includes(slug));
});

// What the text already embeds is not listed again below it.
const embedded = [
    ...findReferences(page.value.body, 'topic-ref').map((id) => `/topics/${id}`),
    ...findReferences(page.value.body, 'faq-ref').map((id) => `/faq/${id}`),
];
const related = computed(() => (allRelated.value ?? []).filter((entry) => !embedded.includes(entry.path)));
const usedInTopics = computed(() => (backlinks.value?.topics ?? []).filter((topic) => {
    return !embedded.includes(topic.path) && !related.value.some((entry) => entry.path === topic.path);
}));

const { data: surround } = await useAsyncData(`${props.collection}-${slug}-surround`, async () => {
    const pages = await queryCollection(props.collection).order('title', 'ASC').select('path', 'title', 'description').all();
    const index = pages.findIndex((entry) => entry.path === path);
    const short = (entry) => entry && { path: entry.path, title: entry.title, description: isFaq ? undefined : entry.description };
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

const relatedPart = LEARN_PARTS[isFaq ? 'topics' : 'faq'];

const literatureSortKey = (entry) => entry.author?.[0] ?? entry.title;
const literature = computed(() => (page.value.literature ?? []).toSorted((a, b) => literatureSortKey(a).localeCompare(literatureSortKey(b))));

const hasToc = computed(() => !isFaq && (page.value.body?.toc?.links?.length ?? 0) > 1);

const metaItems = computed(() => [
    !isFaq && page.value.authors?.length && { icon: 'lucide:pen-line', text: page.value.authors.join(', '), title: t('learnAuthor') },
    page.value.date && { icon: 'lucide:calendar', text: d(new Date(page.value.date), LEARN_DATE_FORMAT) },
    lastChangeItem(page.value, t, d),
].filter(Boolean));

useSeoMeta({
    title: page.value.title,
    description: isFaq ? page.value.answer : page.value.description,
});
</script>

<template>
    <UContainer>
        <UPage :ui="hasToc ? { root: 'lg:grid-cols-[280px_minmax(0,1fr)_220px]', left: 'lg:col-span-1', center: 'lg:col-span-1', right: 'lg:col-span-1' } : { root: 'lg:grid-cols-[280px_minmax(0,1fr)]', left: 'lg:col-span-1', center: 'lg:col-span-1' }">
            <template #left>
                <UPageAside>
                    <LearnNavigation />
                </UPageAside>
            </template>

            <UPageHeader
                :title="page.title"
                :description="isFaq ? undefined : page.description"
                :ui="{ title: isFaq ? 'text-2xl sm:text-3xl' : undefined }"
            >
                <template #headline>
                    <UBreadcrumb :items="breadcrumb" :ui="{ linkLabel: 'max-w-64 truncate' }" />
                </template>
                <PageMetaLine :items="metaItems" />
                <p v-if="page.tags?.length" class="mt-3 flex flex-wrap items-center gap-1.5">
                    <UBadge v-for="tag in page.tags" :key="tag" :label="tag" :to="{ path: tagsTo, query: { tag } }" as="NuxtLink" color="neutral" variant="outline" class="hover:bg-elevated" />
                </p>
            </UPageHeader>

            <UPageBody>
                <div v-if="isFaq" class="rounded-r-lg border-l-4 px-5 py-4" :class="ANSWER_CLASSES[page.verdict] ?? 'border-accented bg-elevated/50'">
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

                <section v-if="page.literature?.length">
                    <h2 class="mb-3 text-lg font-semibold text-highlighted">{{ t('learnLiterature') }}</h2>
                    <ul class="space-y-2 text-sm">
                        <li v-for="entry in literature" :key="literatureSortKey(entry) + entry.title" class="pl-6 -indent-6 text-pretty">
                            <LiteratureEntry :entry="entry" />
                        </li>
                    </ul>
                </section>

                <USeparator />

                <section v-if="related?.length">
                    <h2 class="mb-3 text-lg font-semibold text-highlighted">{{ isFaq ? t('faqMoreInTopics') : t('topicQuestions') }}</h2>
                    <ul class="divide-y divide-default rounded-lg border border-default">
                        <li v-for="item in related" :key="item.path">
                            <NuxtLink :to="item.path" class="group flex gap-3 p-3 hover:bg-elevated/60">
                                <UIcon :name="relatedPart.icon" class="mt-1 size-4 shrink-0" :class="relatedPart.color" />
                                <span>
                                    <span class="block font-medium text-highlighted group-hover:text-primary">{{ item.title }}</span>
                                    <span class="block text-sm text-muted text-pretty">{{ isFaq ? item.description : item.answer }}</span>
                                </span>
                            </NuxtLink>
                        </li>
                    </ul>
                </section>

                <section v-if="backlinks?.lessons.length || usedInTopics.length">
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
                        <UButton
                            v-for="topic in usedInTopics"
                            :key="topic.path"
                            :to="topic.path"
                            :label="topic.title"
                            :icon="LEARN_PARTS.topics.icon"
                            :ui="{ leadingIcon: LEARN_PARTS.topics.color }"
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

            <template v-if="hasToc" #right>
                <UContentToc :links="page.body.toc.links" :title="t('learnToc')" highlight />
            </template>
        </UPage>
        <QueryDrawer v-if="page.queries?.length" v-model:open="queryDrawerOpen" :query="activeQuery" sort-by="density" />
    </UContainer>
</template>
