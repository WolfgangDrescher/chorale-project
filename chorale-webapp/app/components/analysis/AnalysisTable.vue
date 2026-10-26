<script setup>
const props = defineProps({
    // The result of an analysis, or one of the variants it states (see analyses/README.md).
    analysis: { type: Object, required: true },
    labels: { type: Array, default: () => [] },
});

const { t } = useI18n();
const format = inject('analysisFormat');

function groupsOf(parent) {
    const groups = (parent.groups ?? []).map((group) => ({ ...group, share: group.matchCount / parent.matchCount }));
    if (parent.omittedMatchCount) {
        groups.push({ other: true, matchCount: parent.omittedMatchCount, share: parent.omittedMatchCount / parent.matchCount });
    }
    return groups;
}

const rows = computed(() => groupsOf(props.analysis));
// Whether a row opens, which leaves room for a chevron in the rows beside it that don't.
const indented = computed(() => rows.value.some((row) => row.groups?.length || row.members?.length));

const FIRST_ROWS = 10;
// The bars above the table show all of them for their gray bar of the rest (see AnalysisBarChart.vue).
const showAll = defineModel('showAll', { type: Boolean, default: false });
const shownRows = computed(() => (showAll.value ? rows.value : rows.value.slice(0, FIRST_ROWS)));

const expanded = ref(new Set());

function toggle(index) {
    const next = new Set(expanded.value);
    if (next.has(index)) next.delete(index);
    else next.add(index);
    expanded.value = next;
}

const drawerOpen = ref(false);
const activeQuery = ref('');

function openQuery(query) {
    activeQuery.value = JSON.stringify(query, null, 4);
    drawerOpen.value = true;
}
</script>

<template>
    <div class="my-6">
        <table class="w-full table-fixed text-sm">
                <AnalysisColumns />
                <thead>
                    <tr class="border-b border-default text-left text-muted">
                        <th class="py-2 pr-4 font-medium">{{ labels[0] || t('analysisGroup') }}</th>
                        <th class="py-2 pr-4 text-right font-medium">{{ t('analysisMatches') }}</th>
                        <th class="py-2 pr-4 font-medium">{{ t('analysisShare') }}</th>
                        <th class="py-2 pr-4 text-right font-medium">{{ t('analysisChorales') }}</th>
                        <th class="py-2 font-medium"><span class="sr-only">{{ t('query') }}</span></th>
                    </tr>
                </thead>
                <tbody>
                    <template v-for="(row, index) in shownRows" :key="index">
                        <AnalysisGroupRow
                            :group="row"
                            :expandable="!!row.groups?.length"
                            :expanded="expanded.has(index)"
                            :indent="indented"
                            @toggle="toggle(index)"
                            @query="openQuery"
                        />
                        <tr v-if="expanded.has(index)" class="border-b border-default">
                            <td colspan="5" class="pb-3">
                                <div class="ml-5 border-l-2 border-primary/40 pl-4">
                                    <table class="w-full table-fixed text-sm">
                                        <AnalysisColumns />
                                        <thead>
                                            <tr class="border-b border-default text-left text-xs text-muted">
                                                <th class="py-1.5 pr-4 font-medium">{{ labels[1] || t('analysisGroup') }}</th>
                                                <th class="py-1.5 pr-4 text-right font-medium">{{ t('analysisMatches') }}</th>
                                                <th class="py-1.5 pr-4 font-medium">{{ t('analysisShareOf', { label: format.label(row.label) }) }}</th>
                                                <th class="py-1.5 pr-4 text-right font-medium">{{ t('analysisChorales') }}</th>
                                                <th class="py-1.5 font-medium"><span class="sr-only">{{ t('query') }}</span></th>
                                            </tr>
                                        </thead>
                                        <tbody>
                                            <AnalysisGroupRow
                                                v-for="(group, groupIndex) in groupsOf(row)"
                                                :key="groupIndex"
                                                :group="group"
                                                :indent="groupsOf(row).some((group) => group.members?.length)"
                                                nested
                                                @query="openQuery"
                                            />
                                        </tbody>
                                    </table>
                                </div>
                            </td>
                        </tr>
                    </template>
                </tbody>
                <tfoot v-if="rows.length > FIRST_ROWS">
                    <tr>
                        <td colspan="5" class="pt-3 text-center">
                            <UButton
                                :icon="showAll ? 'lucide:chevron-up' : 'lucide:chevron-down'"
                                :label="showAll ? t('analysisShowFirst', { count: FIRST_ROWS }) : t('analysisShowAll', { count: rows.length })"
                                color="neutral"
                                variant="soft"
                                size="xs"
                                @click="showAll = !showAll"
                            />
                        </td>
                    </tr>
                </tfoot>
        </table>
        <p class="mt-2 flex items-center gap-2 text-sm text-muted">
            {{ t('analysisTotal', { matches: analysis.matchCount, count: analysis.choraleCount }) }}
            <UButton
                icon="lucide:search"
                color="neutral"
                variant="soft"
                size="xs"
                :label="t('query')"
                @click="openQuery(analysis.query)"
            />
        </p>
        <QueryDrawer v-model:open="drawerOpen" :query="activeQuery" sort-by="density" />
    </div>
</template>
