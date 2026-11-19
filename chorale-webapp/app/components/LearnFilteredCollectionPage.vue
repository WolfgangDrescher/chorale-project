<script setup>
// The default slot renders the entries that pass the search and the tags.
const props = defineProps({
    collection: { type: String, required: true },
    title: { type: String, required: true },
    description: { type: String, default: '' },
});

const settings = LEARN_COLLECTION_PAGES[props.collection];
const { t } = useI18n();
const localePath = useLocalePath();
const route = useRoute();

// The bodies are not sent to the browser.
const { data: entries } = await useAsyncData(`${props.collection}-index`, async () => {
    const pages = await queryCollection(props.collection).order('title', 'ASC').all();
    return pages.map(({ body, ...page }) => ({ ...page, slug: slugOfPath(page.path) }));
});

const { data: lessonNumbersBySlug } = await useAsyncData(`${props.collection}-index-lessons`, async () => {
    const lessons = await queryCollection('lessons').where('lessonNumber', 'IS NOT NULL').order('lessonNumber', 'ASC').select('lessonNumber', 'body').all();
    const bySlug = {};
    for (const lesson of lessons) {
        for (const id of findReferences(lesson.body, settings.reference)) (bySlug[id] ??= []).push(lesson.lessonNumber);
    }
    return bySlug;
}, { default: () => ({}) });

const searchTerm = ref('');
const activeTags = ref([route.query.tag].flat().filter(Boolean));
const isFiltered = computed(() => !!searchTerm.value || activeTags.value.length > 0);

function reset() {
    searchTerm.value = '';
    activeTags.value = [];
}

const tags = computed(() => {
    const counts = {};
    for (const entry of entries.value ?? []) for (const tag of entry.tags ?? []) counts[tag] = (counts[tag] ?? 0) + 1;
    return Object.entries(counts).sort((a, b) => b[1] - a[1] || a[0].localeCompare(b[0])).map(([tag, count]) => ({ tag, count }));
});

function toggleTag(tag) {
    activeTags.value = activeTags.value.includes(tag) ? activeTags.value.filter((active) => active !== tag) : [...activeTags.value, tag];
}

const SORT_VALUES = {
    title: (entry) => normalizeForSearch(entry.title),
    date: (entry) => String(entry.date ?? ''),
};
const SORT_ICONS = { asc: 'lucide:arrow-up-narrow-wide', desc: 'lucide:arrow-down-wide-narrow' };
// Newest first.
const startsDescending = (id) => id !== 'title';
const sorting = ref({ id: settings.sortBy[0], desc: startsDescending(settings.sortBy[0]) });
const sortItems = computed(() => settings.sortBy.map((id) => ({ label: t(`learnSort.${id}`), value: id })));
const sortId = computed({
    get: () => sorting.value.id,
    set: (id) => (sorting.value = { id, desc: startsDescending(id) }),
});
const toggleSortDirection = () => (sorting.value = { ...sorting.value, desc: !sorting.value.desc });

const filtered = computed(() => {
    const words = normalizeForSearch(searchTerm.value).split(/\s+/).filter(Boolean);
    const valueOf = SORT_VALUES[sorting.value.id];
    const compare = (a, b) => valueOf(a).localeCompare(valueOf(b));
    return (entries.value ?? [])
        .filter((entry) => {
            const text = normalizeForSearch([entry.title, entry.description, entry.answer, ...(entry.tags ?? [])].filter(Boolean).join(' '));
            return words.every((word) => text.includes(word)) && activeTags.value.every((tag) => entry.tags?.includes(tag));
        })
        .sort((a, b) => (sorting.value.desc ? -1 : 1) * compare(a, b) || a.title.localeCompare(b.title));
});

useSeoMeta({ title: props.title, description: props.description });
</script>

<template>
    <UContainer>
        <UPage :ui="{ root: 'lg:grid-cols-[280px_minmax(0,1fr)]', left: 'lg:col-span-1', center: 'lg:col-span-1' }">
            <template #left>
                <ResponsivePageAside :title="t('learn')">
                    <LearnNavigation />
                </ResponsivePageAside>
            </template>

            <UPageHeader :title="title" :description="description">
                <template #headline>
                    <UBreadcrumb :items="[{ label: t('learn'), to: localePath('/learn') }]" />
                </template>
            </UPageHeader>

            <UPageBody>
                <UEmpty v-if="!entries.length" :icon="LEARN_PARTS[collection].icon" :title="t('learnEmpty')" class="mx-auto md:w-1/2" />
                <div v-else class="flex flex-col gap-4">
                    <div class="flex flex-wrap items-center justify-between gap-3">
                        <UInput v-model="searchTerm" icon="lucide:search" :placeholder="t(settings.searchPlaceholder)" size="lg" class="w-full max-w-sm">
                            <template v-if="isFiltered" #trailing>
                                <UButton icon="lucide:x" color="neutral" variant="link" size="sm" :aria-label="t('learnResetFilter')" :title="t('learnResetFilter')" @click="reset" />
                            </template>
                        </UInput>
                        <div class="flex items-center gap-2">
                            <USelect v-if="settings.sortBy.length > 1" v-model="sortId" :items="sortItems" size="sm" class="w-28 sm:w-36" :aria-label="t('sortBy')" />
                            <UButton
                                :icon="SORT_ICONS[sorting.desc ? 'desc' : 'asc']"
                                color="neutral"
                                variant="outline"
                                size="sm"
                                :aria-label="t('sortDirection')"
                                @click="toggleSortDirection"
                            />
                        </div>
                    </div>
                    <div class="flex flex-wrap gap-2">
                        <UButton
                            v-for="{ tag, count } in tags"
                            :key="tag"
                            size="sm"
                            :color="activeTags.includes(tag) ? 'primary' : 'neutral'"
                            :variant="activeTags.includes(tag) ? 'solid' : 'outline'"
                            :aria-label="t('learnTagCount', { tag, count })"
                            @click="toggleTag(tag)"
                        >
                            {{ tag }}
                            <span class="ml-1 text-xs font-normal tabular-nums" :class="activeTags.includes(tag) ? 'opacity-80' : 'text-dimmed'">{{ count }}</span>
                        </UButton>
                    </div>
                    <p class="text-sm text-muted">{{ t(settings.count, { n: filtered.length, total: entries.length }) }}</p>
                </div>

                <UEmpty
                    v-if="entries.length && !filtered.length"
                    icon="lucide:search"
                    :title="t('learnNoResults')"
                    :description="t('learnNoResultsDescription')"
                    :actions="[{ icon: 'lucide:x', label: t('learnResetFilter'), color: 'neutral', variant: 'outline', onClick: reset }]"
                    class="mx-auto md:w-1/2"
                />
                <slot v-else-if="filtered.length" :entries="filtered" :lesson-numbers-by-slug="lessonNumbersBySlug" />
            </UPageBody>
        </UPage>
    </UContainer>
</template>
