<script setup>
// The search palette (meta+K, or the button in the header): the full text of the docs and the
// lessons, and the chorales by their number, title, ID or BWV number.
const { t } = useI18n();
const localePath = useLocalePath();
const { open } = useContentSearch();

// The text of all docs and lessons is loaded in the browser the first time the palette opens,
// not sent along with every page.
const lazy = { server: false, immediate: false, default: () => [] };

const { data: files, execute: loadFiles } = useLazyAsyncData('search-sections', async () => {
    const [docs, lessons] = await Promise.all([
        queryCollectionSearchSections('docs'),
        queryCollectionSearchSections('lessons'),
    ]);
    return [...docs, ...lessons];
}, lazy);

const { data: navigation, execute: loadNavigation } = useLazyAsyncData('search-navigation', async () => {
    const [docs, lessons] = await Promise.all([
        queryCollectionNavigation('docs'),
        queryCollectionNavigation('lessons', ['lessonNumber']),
    ]);
    // The results are grouped by the title of each collection's index page, which for the docs
    // is the name of the search engine.
    return [...docs.map((item) => ({ ...item, title: t('docs') })), ...withLessonNumbers(lessons)];
}, lazy);

const { data: chorales, execute: loadChorales } = useLazyAsyncData('search-chorales', () => {
    return queryCollection('chorales').select('choraleId', 'title', 'number', 'bwv').order('stem', 'ASC').all();
}, lazy);

watch(open, (isOpen) => {
    if (isOpen) {
        loadFiles();
        loadNavigation();
        loadChorales();
    }
}, { once: true });

// A lesson is listed with its number in front of its title, as in the navigation of the lessons.
function withLessonNumbers(items) {
    return items.map((item) => ({
        ...item,
        title: item.lessonNumber ? `${item.lessonNumber}. ${item.title}` : item.title,
        children: item.children ? withLessonNumbers(item.children) : item.children,
    }));
}

const groups = computed(() => [
    {
        id: 'chorales',
        label: t('chorales'),
        items: chorales.value.map((chorale) => ({
            label: chorale.title ? [chorale.number, chorale.title].filter(Boolean).join('. ') : chorale.choraleId,
            suffix: [chorale.choraleId, chorale.bwv].filter(Boolean).join(' · '),
            icon: 'lucide:music',
            to: localePath({ name: 'chorale-id', params: { id: chorale.choraleId } }),
        })),
        // All 370 chorales would fill the palette before anything is typed.
        postFilter: (searchTerm, items) => (searchTerm ? items : []),
    },
]);
</script>

<template>
    <ClientOnly>
        <LazyUContentSearch
            :files="files"
            :navigation="navigation"
            :groups="groups"
            :placeholder="t('searchPlaceholder')"
            :color-mode="false"
            :content="{ onCloseAutoFocus: (event) => event.preventDefault() }"
        />
    </ClientOnly>
</template>
