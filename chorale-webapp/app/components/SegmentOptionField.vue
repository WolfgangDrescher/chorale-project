<script setup>
// One option of the segment analysis as a form field: a select or a switch, with its help in a
// popover next to the label (see CHECK_OPTIONS in components/SegmentAnalysis.vue for what `option` holds).
const props = defineProps({
    option: { type: Object, required: true },
});

const model = defineModel({ type: [Number, String, Boolean], default: undefined });

const localePath = useLocalePath();

// The badge at the top of the help, linking to the docs: an option that is a query option too
// names it as `query` (the docs' headings are the option names, lowercased), any other explains
// itself in the docs somewhere else and says where as `docs`.
const docsBadge = computed(() => {
    const { docs, query } = props.option;
    if (docs) return { label: docs.label, to: localePath({ path: docs.path, hash: docs.hash ? `#${docs.hash}` : undefined }) };
    if (query) return { label: query, to: localePath({ path: '/docs/options', hash: `#${query.toLowerCase()}` }) };
    return null;
});
</script>

<template>
    <UFormField>
        <template #label>
            <span class="inline-flex items-center gap-1">
                {{ $t(option.label) }}
                <UPopover :content="{ side: 'top' }" arrow>
                    <UButton
                        color="neutral"
                        variant="link"
                        size="xs"
                        icon="lucide:info"
                        class="p-0"
                        :aria-label="$t('optionHelp', { option: $t(option.label) })"
                    />
                    <template #content>
                        <div class="max-w-xs p-3 text-sm">
                            <ULink v-if="docsBadge" :to="docsBadge.to" raw class="inline-block mb-2">
                                <UBadge color="neutral" variant="subtle" icon="lucide:book-open" :label="docsBadge.label" class="font-mono" />
                            </ULink>
                            <p>{{ $t(option.description) }}</p>
                            <p v-if="option.disabled" class="mt-2 text-dimmed">{{ $t('optionDisabled') }}</p>
                        </div>
                    </template>
                </UPopover>
            </span>
        </template>
        <USelect v-if="option.type === 'select'" v-model="model" :items="option.items" class="w-full" />
        <USwitch v-else-if="option.disabled" :model-value="true" disabled />
        <USwitch v-else v-model="model" />
    </UFormField>
</template>
