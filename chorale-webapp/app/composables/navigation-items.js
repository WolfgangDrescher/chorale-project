export function useNavigationItems() {
    const { t } = useI18n();
    const localePath = useLocalePath();
    const route = useRoute();

    return computed(() => [
        {
            label: t('home'),
            to: localePath({ name: 'index' }),
        },
        {
            label: t('chorales'),
            to: localePath({ name: 'chorale' }),
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
            // Also on the page of data of a lesson, which is a route of its own.
            active: route.path.startsWith(localePath('/lessons')),
        },
        {
            label: t('docs'),
            to: localePath('/docs'),
        },
    ]);
}
