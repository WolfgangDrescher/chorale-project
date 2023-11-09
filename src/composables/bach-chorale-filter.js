const filterByTitle = (title, element) => {
    if (!title) return true;
    return element.title?.toLowerCase().includes(title?.toLowerCase());
};

const filterByNr = (nr, element) => {
    if (!nr) return true;
    return parseInt(element.nr, 10) === parseInt(nr, 10);
};

const filterByKeys = (keys, element) => {
    if (!keys.length) return true;
    return keys.includes(element.key);
};

const filterBySearchText = (searchText, element) => {
    if (!searchText) return true;
    return filterByTitle(searchText, element) || filterByNr(searchText, element);
};

const filterByMajorMinor = (majorMinor, element) => {
    switch (majorMinor) {
        case 'major':
            return element.isMajor;
        case 'minor':
            return element.isMinor;
    }
    return true;
};

const filterByCountCadences = ([min, max], element) => {
    if (typeof min !== 'number' || typeof max !== 'number') return true;
    const count = element.countCadences ?? 0;
    // if (isNaN(count)) return false;
    return count >= min && count <= max;
};

export function useBachChoraleFilter(elements) {

    const filter = useBachChoraleFilterStore();

    const filteredElements = computed(() => {
        const filteredElements = elements.filter(element => {
            const searchTextMatched = filterBySearchText(filter.searchText, element);
            const keysMatched = filterByKeys(filter.keys, element);
            const majorMinorMatched = filterByMajorMinor(filter.majorMinor, element);
            const countCadencesMatched = filterByCountCadences(filter.countCadences, element);

            return (
                searchTextMatched &&
                keysMatched &&
                majorMinorMatched &&
                countCadencesMatched
            );
        });

        filteredElements.sort((a, b) => {
            if (filteredElements.orderBy === 'id') {
                return a.id > b.id ? 1 : -1;
            }
            return 0;
        });

        return filteredElements;
    });

    return {
        filteredElements,
    };
}
