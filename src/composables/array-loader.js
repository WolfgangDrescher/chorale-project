export function useArrayLoader(elements, numberOfItemsToAdd = 10) {
    const items = ref([]);
    const elRef = ref(elements);
    watch(elRef, () => {
        items.value = [];
        addItems(numberOfItemsToAdd);
    });
    function addItems(num = numberOfItemsToAdd) {
        items.value.push(...unref(elRef).slice(items.value.length, items.value.length + num));
    }
    addItems(numberOfItemsToAdd);
    return {
        items: readonly(items),
        addItems,
    };
}
