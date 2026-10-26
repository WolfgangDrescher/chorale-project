<script setup>
const props = defineProps({
    result: { type: Object, required: true },
    // The variants that get a bar of their own below the one of all the matches.
    phrases: { type: Array, default: () => [] },
    // By the label of a group, as steps of the colors of the theme: { perfect: primary-600 }.
    colors: { type: Object, default: () => ({}) },
});

const { t } = useI18n();
const format = inject('analysisFormat');

// Without colors of its own, the groups in order as a ramp of the primary color.
const PALETTE = ['primary-700', 'primary-600', 'primary-500', 'primary-400', 'primary-300'];
const color = (name) => `var(--ui-color-${name})`;

// The groups of the first level, in the order of all the matches, so every bar stacks them the same way.
const parts = computed(() => props.result.groups.map((group, index) => ({
    label: group.label,
    color: color(props.colors[group.label] ?? PALETTE[index % PALETTE.length]),
})));

const rows = computed(() => [{ name: '', node: props.result }, ...props.phrases.map((name) => ({ name, node: props.result.variants[name] }))]
    .map(({ name, node }) => ({
        label: t(name ? `analysisVariant.${name}` : 'analysisAllMatches'),
        matchCount: node.matchCount,
        items: [
            ...parts.value.map((part) => ({
                label: format.label(part.label),
                color: part.color,
                value: node.groups.find((group) => group.label === part.label)?.matchCount ?? 0,
            })),
            ...(node.omittedMatchCount ? [{ label: t('analysisOther'), color: color('neutral-300'), value: node.omittedMatchCount }] : []),
        ],
    })));
</script>

<template>
    <div class="space-y-4">
        <div v-for="row in rows" :key="row.label" class="grid grid-cols-[7rem_minmax(0,1fr)] items-start gap-x-4 text-sm">
            <span class="pt-0.5">{{ row.label }}</span>
            <UProgressGroup
                :items="row.items"
                :max="row.matchCount"
                size="lg"
                :ui="{ list: 'flex-row flex-wrap gap-x-4 text-xs text-muted', itemTrailing: 'ms-0' }"
            >
                <template #item-trailing="{ percent }">
                    <span class="tabular-nums">{{ format.percent(percent / 100) }}</span>
                </template>
            </UProgressGroup>
        </div>
    </div>
</template>
