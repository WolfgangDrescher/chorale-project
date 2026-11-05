import { useLocalStorage } from '@vueuse/core';

// Stored for the next visit and for paging through the filtered chorales on their pages.
export function useChoraleListFilter() {
    const { t } = useI18n();

    const filter = useLocalStorage('chorale-list-filter', '', { initOnMounted: true });
    const listFilter = computed(() => parseChoraleListFilter(filter.value));

    // What the filter was understood as, so it is clear why a chorale is or is not listed.
    const listFilterBadges = computed(() => {
        const { meters, key, modes, words } = listFilter.value;
        return [
            ...meters.map((meter) => ({ label: t('meter'), value: meter })),
            ...(key ? [{ label: t('key'), value: [key.key.toUpperCase(), key.mode].filter(Boolean).join(' ') }] : []),
            ...modes.map((mode) => ({ label: t('mode'), value: mode })),
            // The words are looked for one by one, which the commas between them show.
            ...(words.length ? [{ label: t('text'), value: words.join(', ') }] : []),
        ];
    });

    const isFiltered = computed(() => listFilterBadges.value.length > 0);

    return { filter, listFilter, listFilterBadges, isFiltered };
}
