import { useLocalStorage } from '@vueuse/core';

// Stored for the next visit and for paging through the filtered chorales on their pages.
export function useChoraleListFilter() {
    const filter = useLocalStorage('chorale-list-filter', '', { initOnMounted: true });
    const listFilter = computed(() => parseChoraleListFilter(filter.value));
    return { filter, listFilter };
}
