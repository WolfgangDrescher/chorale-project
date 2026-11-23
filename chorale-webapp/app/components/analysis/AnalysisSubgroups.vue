<script setup>
const props = defineProps({
    parent: { type: Object, required: true },
    // The values of the parent and its ancestors at each level.
    path: { type: Array, required: true },
    // 0 is the first level; picks the column label.
    level: { type: Number, required: true },
    labels: { type: Array, default: () => [] },
    showsMatches: { type: Boolean, default: false },
});

const emit = defineEmits(['query', 'matches']);

const { t } = useI18n();
const format = inject('analysisFormat');

const rows = computed(() => analysisGroupsOf(props.parent));
// Whether a row opens, which leaves room for a chevron in the rows beside it that don't.
const indented = computed(() => rows.value.some((row) => row.groups?.length || row.members?.length));

const expanded = ref(new Set());

function toggle(index) {
    const next = new Set(expanded.value);
    if (next.has(index)) next.delete(index);
    else next.add(index);
    expanded.value = next;
}
</script>

<template>
    <div class="ml-5 border-l-2 border-primary/40 pl-4">
        <table class="w-full table-fixed text-sm">
            <AnalysisColumns />
            <thead>
                <tr class="border-b border-default text-left text-xs text-muted">
                    <th class="py-1.5 pr-4 font-medium">{{ labels[level] || format.levelTitle(level) }}</th>
                    <th class="py-1.5 pr-4 text-right font-medium">{{ t('analysisMatches') }}</th>
                    <th class="py-1.5 pr-4 font-medium">{{ t('analysisShareOf', { label: format.label(parent.label) }) }}</th>
                    <th class="py-1.5 pr-4 text-right font-medium">{{ t('analysisChorales') }}</th>
                    <th class="py-1.5 font-medium"><span class="sr-only">{{ t('query') }}</span></th>
                </tr>
            </thead>
            <tbody>
                <template v-for="(row, index) in rows" :key="index">
                    <AnalysisGroupRow
                        :group="row"
                        :expandable="!!row.groups?.length"
                        :expanded="expanded.has(index)"
                        :indent="indented"
                        :level="level"
                        nested
                        :shows-matches="showsMatches"
                        @toggle="toggle(index)"
                        @query="emit('query', $event)"
                        @matches="(member) => emit('matches', [...path, row.values], member)"
                    />
                    <tr v-if="expanded.has(index)" class="border-b border-neutral-100 last:border-0 dark:border-neutral-900">
                        <td colspan="5" class="pb-3">
                            <AnalysisSubgroups
                                :parent="row"
                                :path="[...path, row.values]"
                                :level="level + 1"
                                :labels="labels"
                                :shows-matches="showsMatches"
                                @query="emit('query', $event)"
                                @matches="(rowPath, member) => emit('matches', rowPath, member)"
                            />
                        </td>
                    </tr>
                </template>
            </tbody>
        </table>
    </div>
</template>
