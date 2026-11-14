<script setup>
// The ids of the options chosen, or null for all of them. Kept from chorale to chorale.
const selected = defineModel({ type: Array, default: null });

const props = defineProps({
    // A filter group of a collection, see chorale-collections.js.
    group: { type: Object, required: true },
});

const { t } = useI18n();

const shownIds = computed(() => {
    const ids = props.group.options.map(({ id }) => id);
    const chosen = ids.filter((id) => selected.value?.includes(id));
    return chosen.length ? chosen : ids;
});
const showsAll = computed(() => shownIds.value.length === props.group.options.length);

function toggle(id, event) {
    const ids = clickSelection(shownIds.value, id, event);
    selected.value = ids.length && ids.length < props.group.options.length ? ids : null;
}

const buttons = computed(() => [
    { id: 'all', label: t('allOptions'), pressed: showsAll.value, select: () => (selected.value = null) },
    ...props.group.options.map((option) => ({
        id: option.id,
        label: option.number === undefined ? t(option.label) : String(option.number),
        pressed: !showsAll.value && shownIds.value.includes(option.id),
        select: (event) => toggle(option.id, event),
    })),
]);
</script>

<template>
    <div
        role="group"
        :aria-label="t(group.label)"
        class="flex w-fit max-w-full items-center gap-0.5 overflow-x-auto rounded-full bg-elevated p-1"
    >
        <span class="flex shrink-0 items-center gap-1.5 self-stretch border-r border-accented px-2.5 sm:px-3 mr-1 text-sm text-dimmed">
            <UIcon :name="group.icon" class="size-4" />
            <span class="hidden sm:inline">{{ t(group.label) }}</span>
        </span>
        <button
            v-for="button in buttons"
            :key="button.id"
            type="button"
            :aria-pressed="button.pressed"
            class="shrink-0 rounded-full px-2 sm:px-3 py-1 text-sm font-medium transition-colors cursor-pointer"
            :class="button.pressed ? 'bg-default text-highlighted shadow-sm' : 'text-muted hover:text-default'"
            @click="button.select($event)"
        >
            {{ button.label }}
        </button>
    </div>
</template>
