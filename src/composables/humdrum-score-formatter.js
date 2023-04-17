export function useHumdrumScoreFormatter(dataProp, filtersProp) {
    const data = ref(dataProp);
    const filters = filtersProp || ref([]);
    // When using exportMode filters are ignored and manualFilters will be used instead
    const manualFilters = ref('');

    function addFilter(filter) {
        if (filter.unique) {
            const matchedFilter = filters.value.find(f => f.className === filter.className);
            if (matchedFilter && filter.changeable) {
                removeFilter(matchedFilter.id);
            } else if (matchedFilter) {
                // throw new Error(`Filter ${filter.className} is unique and already in use.`);
                return;
            }
        }
        filters.value.push(filter);
    }

    function removeFilter(filterId) {
        const filter = filters.value.find(f => f.id === filterId);
        if (filter) {
            filter.beforeRemove();
        }
        filters.value = filters.value.filter(f => f.id !== filterId);
    }

    const filtersAsString = computed(() => {
        const sortedFilters = [...filters.value].sort((a, b) => {
            return a.priority > b.priority ? -1 : 1;
        });
        return sortedFilters.map(f => f.toString()).join('\n');
    });

    const formattedScoreData = computed(() => {
        return `${manualFilters.value || filtersAsString.value}\n${unref(data)}`.replace(/^\s+|\s+$/g, '');
    });

    function setData(value) {
        data.value = value;
    }

    return {
        setData,
        addFilter,
        removeFilter,
        manualFilters,
        filters: readonly(filters),
        filtersAsString: filtersAsString,
        formattedScoreData: formattedScoreData,
    };
}
