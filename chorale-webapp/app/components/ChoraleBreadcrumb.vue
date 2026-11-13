<script setup>
import { useClipboard } from '@vueuse/core';

const props = defineProps({
    // The id of a collection in chorale-collections.js, Bach's included.
    collectionId: { type: String, required: true },
    // The id of the chorale, as in its URL. Its title is the heading.
    choraleId: { type: String, required: true },
});

const { t } = useI18n();
const localePath = useLocalePath();
const { copy, copied } = useClipboard();

const collections = [BACH_CHORALES, ...CHORALE_COLLECTIONS];
const collection = findChoraleCollection(props.collectionId);

// Made once: a page left for the next one renders again, see CollectionChoralePage.
const items = [
    { label: t('chorales'), icon: 'lucide:library', to: localePath({ name: 'chorale' }) },
    { label: collection.name, slot: 'collection' },
    { label: props.choraleId, slot: 'chorale' },
];

// The collection menu opens the chorale list showing only that collection.
const collectionMenuItems = collections.map(({ id, name }) => ({
    label: name,
    to: localePath({ name: 'chorale', query: { collections: id } }),
    current: id === collection.id,
}));
</script>

<template>
    <UBreadcrumb :items="items" class="mb-3" :ui="{ item: 'shrink-0 last:shrink' }">
        <template #collection="{ item }">
            <UDropdownMenu :items="collectionMenuItems" :content="{ align: 'center' }" arrow>
                <button type="button" class="cursor-pointer hover:text-default">
                    {{ item.label }}
                </button>
                <template #item-trailing="{ item: menuItem }">
                    <UIcon v-if="menuItem.current" name="lucide:check" class="size-4 text-primary" />
                </template>
            </UDropdownMenu>
        </template>
        <template #chorale="{ item }">
            <UTooltip :text="copied ? t('copied') : t('copy')" :content="{ side: 'top' }">
                <UButton
                    :label="item.label"
                    :trailing-icon="copied ? 'lucide:copy-check' : 'lucide:copy'"
                    color="neutral"
                    variant="soft"
                    size="xs"
                    class="min-w-0 font-mono"
                    :ui="{ trailingIcon: 'text-dimmed' }"
                    @click="copy(item.label)"
                />
            </UTooltip>
        </template>
    </UBreadcrumb>
</template>
