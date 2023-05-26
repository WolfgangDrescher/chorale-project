export function useArrayLoader(elements, numberOfItemsToAdd = 10) {
    const items = ref([]);
    watch(elements, () => {
        items.value = [];
        addItems(numberOfItemsToAdd);
    });
    function addItems(num = numberOfItemsToAdd) {
        items.value.push(...unref(elements).slice(items.value.length, items.value.length + num));
    }
    addItems(numberOfItemsToAdd);
    return {
        items: readonly(items),
        addItems,
    };
}
