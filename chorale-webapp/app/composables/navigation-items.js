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
            label: t('lessons'),
            to: localePath('/lessons'),
            active: isAt(localePath('/lessons')),
        },
        {
            label: t('docs'),
            to: localePath('/docs'),
        },
    ]);
}
