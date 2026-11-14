<script setup>
// The search palette (meta+K, or the button in the header): the full text of the docs and the
// lessons, and the chorales by their number, title, ID, BWV number or starting key.
const { t } = useI18n();
const localePath = useLocalePath();
const { open, mapNavigationItems, postFilter } = useContentSearch();

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
    return queryCollection('bachChorales').select('choraleId', 'title', 'number', 'bwv', 'key', 'mode').order('stem', 'ASC').all();
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

const choraleItems = computed(() => chorales.value.map((chorale) => ({
    label: chorale.title ? [chorale.number, chorale.title].filter(Boolean).join('. ') : chorale.choraleId,
    suffix: [chorale.choraleId, chorale.bwv, [chorale.key, chorale.mode].filter(Boolean).join(' ')].filter(Boolean).join(' · '),
    icon: 'lucide:music',
    to: localePath({ name: 'chorale-bach-id', params: { id: chorale.choraleId } }),
    choraleNumber: chorale.number,
})));

// The chorale whose number is the search term comes first: `9` lists chor009 before chor096.
function withNumberFirst(searchTerm, items) {
    const number = searchTerm.trim().toLowerCase().replace(/^0+(?=\d)/, '');
    const isNumber = (item) => String(item.choraleNumber ?? '').toLowerCase() === number;
    return [...items.filter(isNumber), ...items.filter((item) => !isNumber(item))];
}

// The chorales with every word of the search term in their label or suffix. Not left to Fuse,
// whose 12 results all groups share.
function searchChorales(searchTerm, items) {
    const words = searchTerm.toLowerCase().split(/\s+/).filter(Boolean);
    const hits = items
        .filter((item) => words.every((word) => `${item.label} ${item.suffix}`.toLowerCase().includes(word)))
        .map((item) => ({ ...item, matches: ['label', 'suffix'].map((key) => ({ key, value: item[key], indices: matchedWordRanges(item[key], words) })) }));
    return withNumberFirst(searchTerm, hits);
}

// The ranges of `text` the words are found in, as Fuse states its matches, so the palette
// highlights them: "vom himmel" in "3. Ach Gott, vom Himmel sieh darein" is [[13, 15], [17, 22]].
function matchedWordRanges(text, words) {
    const lowerText = text.toLowerCase();
    const regions = words.flatMap((word) => {
        const found = [];
        for (let index = lowerText.indexOf(word); index !== -1; index = lowerText.indexOf(word, index + 1)) {
            found.push([index, index + word.length - 1]);
        }
        return found;
    });
    return regions
        .sort((a, b) => a[0] - b[0])
        .reduce((merged, region) => {
            const last = merged.at(-1);
            if (last && region[0] <= last[1] + 1) last[1] = Math.max(last[1], region[1]);
            else merged.push(region);
            return merged;
        }, []);
}

const choraleGroups = computed(() => [
    {
        id: 'chorales',
        label: t('chorales'),
        items: choraleItems.value,
        ignoreFilter: true,
        // All 370 chorales would fill the palette before anything is typed.
        postFilter: (searchTerm, items) => (searchTerm ? searchChorales(searchTerm, items) : []),
    },
]);

// The docs and the lessons as UContentSearch builds them, given here so the chorales come first.
const contentGroups = computed(() => navigation.value.map((group) => ({
    id: group.path,
    label: group.title,
    items: mapNavigationItems(group.children ?? [], files.value),
    postFilter,
})));

const groups = computed(() => [...choraleGroups.value, ...contentGroups.value]);
</script>

<template>
    <ClientOnly>
        <LazyUContentSearch
            :groups="groups"
            :fuse="{ fuseOptions: { tokenMatch: 'all' } }"
            :placeholder="t('searchPlaceholder')"
            :color-mode="false"
            :content="{ onCloseAutoFocus: (event) => event.preventDefault() }"
        />
    </ClientOnly>
</template>
