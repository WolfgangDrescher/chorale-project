import contentHistory from '#build/content-history.mjs';

// "/topics/plagal-cadences" => "plagal-cadences"
export function slugOfPath(path) {
    return path?.split('/').filter(Boolean).at(-1) ?? '';
}

// The ids of all `::topic-ref{id}` or `::faq-ref{id}` in a body, whose nodes are [tag, attributes, ...children].
export function findReferences(body, tag) {
    const ids = [];
    const walk = (node) => {
        if (!Array.isArray(node)) return;
        if (node[0] === tag && node[1]?.id) ids.push(node[1].id);
        node.forEach((child) => walk(child));
    };
    walk(body?.value);
    return [...new Set(ids)];
}

// The lessons and topics that embed the page.
export function useLearnBacklinks(tag, slug) {
    return useAsyncData(`backlinks-${tag}-${slug}`, async () => {
        const [lessons, topics] = await Promise.all([
            queryCollection('lessons').where('lessonNumber', 'IS NOT NULL').order('lessonNumber', 'ASC').select('path', 'title', 'lessonNumber', 'body').all(),
            queryCollection('topics').select('path', 'title', 'body').all(),
        ]);
        const refersHere = (page) => findReferences(page.body, tag).includes(slug);
        const strip = ({ body, ...page }) => page;
        return {
            lessons: lessons.filter(refersHere).map(strip),
            topics: topics.filter((page) => slugOfPath(page.path) !== slug && refersHere(page)).map(strip),
        };
    });
}

export const LEARN_DATE_FORMAT = { dateStyle: 'long', timeZone: 'UTC' };

// The PageMetaLine item for the last commit of a page.
export function lastChangeItem(page, t, d) {
    const lastCommit = contentHistory[page.stem];
    if (!lastCommit) return null;
    const date = d(new Date(lastCommit.date), { dateStyle: 'medium', timeZone: 'UTC' });
    return { icon: 'lucide:history', text: t('learnChanged', { date }), commit: lastCommit.commit };
}

// Settings of LearnFilteredCollectionPage. The first entry of `sortBy` is the default.
export const LEARN_COLLECTION_PAGES = {
    topics: { reference: 'topic-ref', searchPlaceholder: 'topicSearchPlaceholder', count: 'topicCount', sortBy: ['title', 'date'] },
    faq: { reference: 'faq-ref', searchPlaceholder: 'faqSearchPlaceholder', count: 'faqCount', sortBy: ['date', 'title'] },
};

export const LEARN_PARTS = {
    lessons: { icon: 'lucide:graduation-cap', color: 'text-teal-600', tile: 'bg-teal-500/15 text-teal-600' },
    topics: { icon: 'lucide:book-open-text', color: 'text-sky-600', tile: 'bg-sky-500/15 text-sky-600' },
    faq: { icon: 'lucide:message-circle-question-mark', color: 'text-amber-600', tile: 'bg-amber-500/15 text-amber-600' },
};
