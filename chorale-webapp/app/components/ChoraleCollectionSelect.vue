<script setup>
// The ids of the collections chosen.
const collections = defineModel({ type: Array, required: true });

defineProps({
    // The number of chorales of each collection, by its id.
    counts: { type: Object, required: true },
});

const { t } = useI18n();

const isOpen = ref(false);

// Chooses like ScoreFilterGroup, see clickSelection.
const items = computed(() => [BACH_CHORALES, ...CHORALE_COLLECTIONS].map(({ id, name }) => ({
    label: name,
    value: id,
    onSelect: (event) => {
        event.preventDefault();
        const click = event.detail?.originalEvent;
        collections.value = clickSelection(collections.value, id, click);
        if (!isMultiSelectClick(click)) isOpen.value = false;
    },
})));

function choose(ids) {
    collections.value = ids;
    isOpen.value = false;
}

const label = computed(() => (collections.value.length === items.value.length
    ? t('allCollections')
    : items.value.filter(({ value }) => collections.value.includes(value)).map(({ label }) => label).join(', ')));
</script>

<template>
    <USelectMenu
        v-model="collections"
        v-model:open="isOpen"
        :items="items"
        value-key="value"
        multiple
        :search-input="false"
        icon="lucide:library"
        :aria-label="$t('collections')"
        :ui="{ itemTrailing: 'min-w-5 justify-end' }"
    >
        <template #item-label="{ item }">
            <span class="flex justify-between gap-3">
                {{ item.label }}
                <span class="text-xs text-muted tabular-nums">{{ counts[item.value] }}</span>
            </span>
        </template>
        <template #default>
            {{ label }}
        </template>
        <template #content-bottom>
            <div class="flex items-center gap-1 p-1.5 border-t border-default">
                <UButton
                    :label="$t('selectAllCollections')"
                    color="neutral"
                    variant="soft"
                    size="xs"
                    @click="choose(items.map(({ value }) => value))"
                />
                <UButton
                    :label="$t('onlyCollection', { name: BACH_CHORALES.name })"
                    color="neutral"
                    variant="soft"
                    size="xs"
                    @click="choose([BACH_CHORALES.id])"
                />
                <i18n-t keypath="multiSelectShortHint" tag="span" scope="global" class="ml-auto flex items-center gap-1 text-xs text-muted" :title="$t('multiSelectHint')">
                    <template #key>
                        <UKbd value="shift" size="sm" />
                    </template>
                </i18n-t>
            </div>
        </template>
    </USelectMenu>
</template>
