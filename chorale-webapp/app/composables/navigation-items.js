const LEARN_PATHS = ['/learn', '/lessons', '/faq'];

export function useNavigationItems() {
    const { t } = useI18n();
    const localePath = useLocalePath();
    const route = useRoute();

    // Whether the current page is the one at path or below it. The link of an item is active on its own
    // only on a page whose route record it matches, which the page of a chorale or the data of a lesson,
    // records of their own beside the index, is not.
    const isAt = (path) => route.path === path || route.path.startsWith(`${path}/`);

    return computed(() => [
        {
            label: t('home'),
            to: localePath({ name: 'index' }),
        },
        {
            label: t('chorales'),
            to: localePath({ name: 'chorale' }),
            active: isAt(localePath({ name: 'chorale' })),
        },
        {
            label: t('choraleAnalysis'),
            to: localePath({ name: 'chorale-analysis' }),
        },
        {
            label: t('search'),
            to: localePath({ name: 'search' }),
        },
        {
            label: t('learn'),
            to: localePath('/learn'),
            // Rendered as LearnMenu in TheNavigation.
            slot: 'learn',
            active: LEARN_PATHS.some((path) => isAt(localePath(path))),
            children: [
                { label: t('lessons'), icon: LEARN_PARTS.lessons.icon, ui: { linkLeadingIcon: LEARN_PARTS.lessons.color }, to: localePath('/lessons'), active: isAt(localePath('/lessons')) },
                { label: t('faq'), icon: LEARN_PARTS.faq.icon, ui: { linkLeadingIcon: LEARN_PARTS.faq.color }, to: localePath('/faq'), active: isAt(localePath('/faq')) },
            ],
        },
        {
            label: t('docs'),
            to: localePath('/docs'),
        },
    ]);
}
