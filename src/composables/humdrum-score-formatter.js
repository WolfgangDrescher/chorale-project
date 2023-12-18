export function useHumdrumScoreFormatter(kernScore) {
    const data = ref(toRaw(unref(kernScore)));
    const filters = ref([]);
    // When using expertMode filters are ignored and manualFilters will be used instead
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

    function removeFilter(filter) {
        let found;
        if (filter.unique) {
            found = filters.value.find(f => f.className === filter.className);
        } else {
            found = filters.value.find(f => f.id === filter.id);
        }
        if (found) {
            found.beforeRemove();
            filters.value = filters.value.filter(f => f.id !== found.id);
        }
    }

    const filtersAsString = computed(() => {
        const sortedFilters = [...filters.value].sort((a, b) => {
            return a.priority > b.priority ? -1 : 1;
        });
        return sortedFilters.map(f => f.toString()).join('\n');
    });

    const formattedScoreData = computed(() => {
        return `${unref(data)}\n${manualFilters.value || filtersAsString.value}`.replace(/^\s+|\s+$/g, '');
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
