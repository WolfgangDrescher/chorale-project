<script setup>
// The search palette (meta+K, or the button in the header): the full text of the docs and the
// lessons, and the chorales of each collection by their number, title, ID, BWV number or key.
const { t } = useI18n();
const choraleRoute = useChoraleRoute();
const { open, mapNavigationItems, postFilter } = useContentSearch();

// The text of all docs and lessons is loaded in the browser the first time the palette opens,
// not sent along with every page.
const lazy = { server: false, immediate: false, default: () => [] };

const { data: files, execute: loadFiles } = useLazyAsyncData('search-sections', async () => {
    const sections = await Promise.all(['docs', 'lessons', 'topics', 'faq'].map((collection) => queryCollectionSearchSections(collection)));
    return sections.flat();
}, lazy);

const { data: navigation, execute: loadNavigation } = useLazyAsyncData('search-navigation', async () => {
    const [docs, lessons, topics, faq] = await Promise.all([
        queryCollectionNavigation('docs'),
        queryCollectionNavigation('lessons', ['lessonNumber']),
        queryCollectionNavigation('topics'),
        queryCollectionNavigation('faq'),
    ]);
    // The results are grouped by the title of each collection's index page, which for the docs
    // is the name of the search engine.
    // The topics and the questions have no index page to take the title from.
    const group = (title, path, children) => ({ title, path, children });
    return [
        ...docs.map((item) => ({ ...item, title: t('docs') })),
        ...withLessonNumbers(lessons),
        group(t('topics'), '/topics', topics),
        group(t('faq'), '/faq', faq),
    ];
}, lazy);

const { data: chorales, execute: loadChorales } = useLazyAsyncData('search-chorales', () => {
    return queryCollection('bachChorales').select('choraleId', 'title', 'number', 'bwv', 'key', 'mode').order('stem', 'ASC').all();
}, lazy);

const { data: collectionChorales, execute: loadCollectionChorales } = useLazyAsyncData('search-collection-chorales', () => {
    return queryCollection('collectionChorales').select('collection', 'choraleId', 'title', 'number', 'key', 'mode').order('stem', 'ASC').all();
}, lazy);

watch(open, (isOpen) => {
    if (isOpen) {
        loadFiles();
        loadNavigation();
        loadChorales();
        loadCollectionChorales();
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
    to: choraleRoute(BACH_CHORALES.id, chorale.choraleId),
    choraleNumber: chorale.number,
})));

// The chorale whose number is the search term comes first: `9` lists chor009 before chor096.
function withNumberFirst(searchTerm, items) {
    const number = searchTerm.trim().toLowerCase().replace(/^0+(?=\d)/, '');
    const isNumber = (item) => String(item.choraleNumber ?? '').toLowerCase() === number;
    return [...items.filter(isNumber), ...items.filter((item) => !isNumber(item))];
}

const collectionChoraleItems = computed(() => Object.fromEntries(CHORALE_COLLECTIONS.map(({ id }) => [
    id,
    collectionChorales.value.filter((chorale) => chorale.collection === id).map((chorale) => ({
        label: `${chorale.number}. ${chorale.title}`,
        suffix: [t('choraleNumber', { number: chorale.number }), [chorale.key, chorale.mode].filter(Boolean).join(' ')].filter(Boolean).join(' · '),
        icon: 'lucide:music',
        to: choraleRoute(id, chorale.choraleId),
        choraleNumber: chorale.number,
        // Searched, not shown: a long slug of the title.
        choraleId: chorale.choraleId,
    })),
])));

// The chorales with every normalized word of the search term in their label, suffix or ID. Not
// left to Fuse, whose 12 results all groups share.
function searchChorales(searchTerm, items) {
    const words = normalizeForSearch(searchTerm).split(/\s+/).filter(Boolean);
    const hits = items
        .filter((item) => words.every((word) => normalizeForSearch(`${item.label} ${item.suffix} ${item.choraleId ?? ''}`).includes(word)))
        .map((item) => ({ ...item, matches: ['label', 'suffix'].map((key) => ({ key, value: item[key], indices: matchedWordRanges(item[key], words) })) }));
    return withNumberFirst(searchTerm, hits);
}

// The ranges of `text` the normalized words are found in, as Fuse states its matches, so the
// palette highlights them: "prise" in "25. Guds Godhed ville vi priſe" is [[25, 29]].
function matchedWordRanges(text, words) {
    const { normalized, origins } = normalizeForSearchWithOrigins(text);
    const regions = words.flatMap((word) => {
        const found = [];
        for (let index = normalized.indexOf(word); index !== -1; index = normalized.indexOf(word, index + 1)) {
            found.push([origins[index], origins[index + word.length - 1]]);
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

const groupLabel = (id) => `${t('chorales')} · ${findChoraleCollection(id).name}`;

// All the chorales would fill the palette before anything is typed.
const choraleGroups = computed(() => [
    [BACH_CHORALES.id, choraleItems.value],
    ...CHORALE_COLLECTIONS.map(({ id }) => [id, collectionChoraleItems.value[id]]),
].map(([id, items]) => ({
    id: `chorales-${id}`,
    label: groupLabel(id),
    items,
    ignoreFilter: true,
    postFilter: (searchTerm, all) => (searchTerm ? searchChorales(searchTerm, all) : []),
})));

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
            :fuse="{ fuseOptions: { ignoreDiacritics: true, tokenMatch: 'all' } }"
            :placeholder="t('searchPlaceholder')"
            :color-mode="false"
            :content="{ onCloseAutoFocus: (event) => event.preventDefault() }"
        />
    </ClientOnly>
</template>
