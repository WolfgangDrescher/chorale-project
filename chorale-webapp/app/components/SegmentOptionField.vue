<script setup>
// One option of the segment analysis as a form field: a select or a switch, with its help in a
// popover next to the label (see CHECK_OPTIONS in pages/segment-analysis.vue for what `option` holds).
defineProps({
    option: { type: Object, required: true },
});

const model = defineModel({ type: [Number, String, Boolean], default: undefined });
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
                        <p class="max-w-xs p-3 text-sm">{{ $t(option.description) }}</p>
                    </template>
                </UPopover>
            </span>
        </template>
        <USelect v-if="option.type === 'select'" v-model="model" :items="option.items" class="w-full" />
        <USwitch v-else v-model="model" />
    </UFormField>
</template>
