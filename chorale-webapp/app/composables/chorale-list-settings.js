import { useLocalStorage } from '@vueuse/core';

// How the list of chorales is shown, stored for the next visit.
export function useChoraleListSettings() {
    const view = useLocalStorage('chorale-list-view', 'table', { initOnMounted: true });
    const visibleColumns = useLocalStorage(
        'chorale-list-columns',
        { key: true, meter: true, modulations: true, incipit: true },
        { initOnMounted: true, mergeDefaults: true },
    );
    // Shared with the modulation timeline of a chorale.
    const coloring = useLocalStorage('chorale-key-coloring', 'absolute', { initOnMounted: true });

    return { view, visibleColumns, coloring };
}
