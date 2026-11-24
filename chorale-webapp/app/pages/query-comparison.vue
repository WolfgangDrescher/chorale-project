<script setup>
import { useLocalStorage } from '@vueuse/core';

const { t } = useI18n();
const localePath = useLocalePath();
const { localScoreUrlGenerator } = useScoreUrlGenerator();

useHead({
    title: t('queryComparison'),
});

const baseQuery = useLocalStorage('chorale-query-comparison-base', '');
const draftQuery = useLocalStorage('chorale-query-comparison-draft', '');

// The diminution whose annotated notes are the expected result, 'none' for no expectation.
const expectedDiminution = useLocalStorage('chorale-query-comparison-expected', 'none');
const expectedChorales = useExpectedDiminutions(expectedDiminution);
const isAnnotated = (diminutionId) => Object.values(DIMINUTION_ANNOTATIONS).some((diminutions) => diminutions[diminutionId]);

const {
    base, draft, pending, compare, compared, preferredUnit, notesAvailable, unit, hasExpectation, chorales, totals, accuracies,
} = useQueryComparison(baseQuery, draftQuery, expectedChorales);

// Each editor hands its query to the other one: the base to start a draft from, the draft to
// become the base of the next one.
const editors = [
    { label: 'queryComparisonBase', query: baseQuery, side: base, other: draftQuery, handOver: 'queryComparisonUseAsDraft', handOverConfirmation: 'queryComparisonUseAsDraftConfirmation', handOverIcon: 'lucide:arrow-right' },
    { label: 'queryComparisonDraft', query: draftQuery, side: draft, other: baseQuery, handOver: 'queryComparisonUseAsBase', handOverConfirmation: 'queryComparisonUseAsBaseConfirmation', handOverIcon: 'lucide:arrow-left' },
];

// What replaces a query is asked about first, in place of the buttons that do it, unless nothing would be lost.
const confirming = ref(null);

function replaceQueries(replacements, confirmation, then = () => {}) {
    const run = () => {
        replacements.forEach(([query, value]) => (query.value = value));
        then();
    };
    if (replacements.every(([query, value]) => !query.value.trim() || query.value === value)) run();
    else confirming.value = { ...confirmation, run };
}

function confirm() {
    confirming.value.run();
    confirming.value = null;
}

function handOver(editor) {
    replaceQueries([[editor.other, editor.query.value]], { at: editor.label, question: t(editor.handOverConfirmation), action: t(editor.handOver) });
}

function format(query) {
    try {
        query.value = formatQuery(JSON.parse(query.value));
    } catch {}
}

// Both editors start from the queries of a diminution, to change one of them, with its
// annotated notes as the expected result.
const diminutionItems = computed(() => DIMINUTIONS.map(({ id, queries }) => ({
    label: t(`diminution.${id}`),
    onSelect: () => {
        const formatted = formatQuery(queries);
        const name = t(`diminution.${id}`);
        replaceQueries([[baseQuery, formatted], [draftQuery, formatted]], {
            at: 'load',
            question: t('queryComparisonLoadDiminutionConfirmation', { name }),
            action: t('queryComparisonLoadDiminutionAction'),
        }, () => (expectedDiminution.value = isAnnotated(id) ? id : 'none'));
    },
})));

const expectedItems = computed(() => [
    { label: t('queryComparisonNoExpected'), value: 'none' },
    ...DIMINUTIONS.filter(({ id }) => isAnnotated(id)).map(({ id }) => ({ label: t(`diminution.${id}`), value: id })),
]);

const metadata = useChoraleMetadata();
const scoreOptions = useSearchScoreOptions();

// The fill of a mark tells the change. A border marks a note the query judged is wrong about:
// one it finds but is not expected, or one expected that it does not find.
const CHANGE_COLORS = {
    unchanged: 'rgb(100 116 139 / 0.3)',
    lost: highlightColorsByName.red,
    added: highlightColorsByName.green,
    missing: 'transparent',
};
const DEVIATION_COLOR = 'rgb(30 41 59)';
// A score scrolls to the first mark of its first group, so the changes come first.
const CHANGES = ['lost', 'added', 'unchanged', 'missing'];
// The changes that hold what each query finds.
const FINDS = {
    base: { unchanged: true, lost: true, added: false, missing: false },
    draft: { unchanged: true, added: true, lost: false, missing: false },
};

// What is marked in the scores: the changes, or all that one of the queries finds, filled
// yellow. The comparison judges the draft against the expected notes.
const view = ref('comparison');
const viewItems = computed(() => [
    { label: t('queryComparisonView'), value: 'comparison' },
    { label: t('queryComparisonBase'), value: 'base' },
    { label: t('queryComparisonDraft'), value: 'draft' },
]);
const judged = computed(() => (view.value === 'base' ? 'base' : 'draft'));

const deviates = (change, note) => FINDS[judged.value][change] !== note.expected;
const deviationCount = (chorale) => CHANGES.flatMap((change) => chorale[change].filter((note) => deviates(change, note))).length;

// In the view of a query, the changes it finds, and those it does not where it misses an expected note.
function isShown(change, note) {
    if (view.value === 'comparison') return true;
    return FINDS[view.value][change] || (hasExpectation.value && note.expected);
}

function fillColor(change) {
    if (view.value === 'comparison') return CHANGE_COLORS[change];
    return FINDS[view.value][change] ? highlightColorsByName.yellow : 'transparent';
}

// An expectation decides what is compared: its chorales, note by note.
const unitItems = computed(() => [
    { label: t('queryComparisonUnitNote'), value: 'note' },
    { label: t('queryComparisonUnitMatch'), value: 'match', disabled: hasExpectation.value },
]);

// How each query does against the expected notes: a note is wrong where it finds one too many
// or misses one, and the expected notes are those that are right or missing.
const accuracySides = computed(() => (accuracies.value ? [
    { label: t('queryComparisonBase'), ...accuracies.value.base },
    { label: t('queryComparisonDraft'), ...accuracies.value.draft },
].map((side) => ({ ...side, wrong: side.falsePositives + side.falseNegatives })) : []));
const expectedNoteCount = computed(() => accuracies.value ? accuracies.value.base.truePositives + accuracies.value.base.falseNegatives : 0);
const accuracyRows = computed(() => [
    { label: t('queryComparisonCorrect'), key: 'truePositives' },
    { label: t('queryComparisonSurplus'), key: 'falsePositives' },
    { label: t('queryComparisonMissing'), key: 'falseNegatives' },
]);

const onlyChanges = ref(true);
const page = ref(1);
// The order of the chorales: by their id, or by how many units changed, were lost, were added
// or deviate from the expected notes in each, most first. Chorales that tie stay in the order of their ids.
const sortKey = ref('id');
const sortDescending = ref(false);
watch(sortKey, (value) => (sortDescending.value = value !== 'id'));
const sortItems = computed(() => [
    { label: t('sortById'), value: 'id' },
    { label: t('queryComparisonSortChanges'), value: 'changes' },
    { label: t('queryComparisonSortLost'), value: 'lost' },
    { label: t('queryComparisonSortAdded'), value: 'added' },
    ...(hasExpectation.value ? [{ label: t('queryComparisonSortDeviations'), value: 'deviations' }] : []),
]);
const SORT_COUNTS = {
    changes: ({ lost, added }) => lost.length + added.length,
    lost: ({ lost }) => lost.length,
    added: ({ added }) => added.length,
    deviations: deviationCount,
};
watch(hasExpectation, (has) => {
    if (!has && sortKey.value === 'deviations') sortKey.value = 'id';
});

const hasDifferences = (chorale) => chorale.lost.length || chorale.added.length || (hasExpectation.value && deviationCount(chorale));
const listedChorales = computed(() => {
    const listed = chorales.value.filter((chorale) => !onlyChanges.value || hasDifferences(chorale));
    if (sortKey.value === 'id') return sortDescending.value ? listed.reverse() : listed;
    const count = SORT_COUNTS[sortKey.value];
    return listed.sort((x, y) => (sortDescending.value ? -1 : 1) * (count(x) - count(y)));
});
const pagedChorales = computed(() => listedChorales.value.slice((page.value - 1) * CHORALES_PER_PAGE, page.value * CHORALES_PER_PAGE));
watch([listedChorales, view], () => (page.value = 1));

const players = useScorePlayers(() => pagedChorales.value.map(({ choraleId }) => choraleId));
// Falls back to the first chorale of the page.
useScorePlayerKeyboardShortcuts(() => players.value.values().next().value);

const noteId = ({ voice, line }) => `L${line}F${voice}`;

// The units of a chorale as the groups a score marks: sections for matches, notes for notes,
// which with expected notes are a group for each change, with and without a border.
function highlightGroups(chorale) {
    const shown = (change) => chorale[change].filter((found) => isShown(change, found));
    if (unit.value === 'match') return { sections: CHANGES.map((change) => ({ color: fillColor(change), items: shown(change) })) };
    if (!hasExpectation.value) return { notes: CHANGES.map((change) => ({ color: fillColor(change), items: shown(change).map(noteId) })) };
    return {
        notes: CHANGES.flatMap((change) => [false, true].map((deviating) => ({
            color: fillColor(change),
            borderColor: deviating ? DEVIATION_COLOR : undefined,
            items: shown(change).filter((note) => deviates(change, note) === deviating).map(noteId),
        }))),
    };
}
</script>

<template>
    <UContainer>
        <Heading>{{ $t('queryComparison') }}</Heading>
        <p class="-mt-2 mb-4 text-sm text-muted">{{ $t('queryComparisonDescription') }}</p>

        <UCard class="mb-4">
            <UForm class="space-y-4" @submit="compare">
                <div class="grid gap-4 lg:grid-cols-2">
                    <UFormField v-for="editor in editors" :key="editor.label" :label="$t(editor.label)">
                        <template #hint>
                            <ConfirmationPrompt
                                v-if="confirming?.at === editor.label"
                                :question="confirming.question"
                                :action="confirming.action"
                                @confirm="confirm"
                                @cancel="confirming = null"
                            />
                            <div v-else class="flex gap-1">
                                <UButton color="neutral" variant="ghost" size="sm" icon="lucide:wand-sparkles" :label="$t('formatQuery')" @click="format(editor.query)" />
                                <UTooltip :text="$t(editor.handOver)">
                                    <UButton color="neutral" variant="ghost" size="sm" :icon="editor.handOverIcon" :aria-label="$t(editor.handOver)" @click="handOver(editor)" />
                                </UTooltip>
                            </div>
                        </template>
                        <MonacoEditor
                            v-model="editor.query.value"
                            :schema="choraleSearchQuerySchema"
                            :min-height="400"
                            :options="{ fontSize: 13, scrollBeyondLastLine: false, automaticLayout: true, minimap: { enabled: false }, scrollbar: { alwaysConsumeMouseWheel: false } }"
                        />
                        <UAlert v-if="editor.side.error.value" color="error" variant="subtle" :title="editor.side.error.value.data?.message ?? $t('searchError')" class="mt-2">
                            <template v-if="editor.side.error.value.data?.errors?.length" #description>
                                <ul>
                                    <li v-for="(message, index) in editor.side.error.value.data.errors" :key="index">{{ message }}</li>
                                </ul>
                            </template>
                        </UAlert>
                    </UFormField>
                </div>
                <ConfirmationPrompt
                    v-if="confirming?.at === 'load'"
                    :question="confirming.question"
                    :action="confirming.action"
                    @confirm="confirm"
                    @cancel="confirming = null"
                />
                <div v-else class="flex flex-wrap gap-2">
                    <UButton type="submit" :loading="pending" :trailing="true">{{ $t('queryComparisonSubmit') }}</UButton>
                    <UDropdownMenu :items="diminutionItems">
                        <UButton color="neutral" variant="subtle" icon="lucide:folder-open" trailing-icon="lucide:chevron-down" :label="$t('queryComparisonLoadDiminution')" />
                    </UDropdownMenu>
                    <ClientOnly>
                        <UFormField :label="$t('queryComparisonExpected')" orientation="horizontal" class="ml-auto items-center gap-2">
                            <USelect v-model="expectedDiminution" :items="expectedItems" class="w-80" />
                        </UFormField>
                    </ClientOnly>
                </div>
            </UForm>
        </UCard>

        <template v-if="compared">
            <div class="flex flex-wrap items-center gap-x-4 gap-y-2 my-4 text-sm text-muted">
                <span class="flex items-center gap-1.5"><span class="size-2.5 rounded-full" :style="{ backgroundColor: CHANGE_COLORS.lost }" />{{ totals.lost }} {{ $t('queryComparisonLost') }}</span>
                <span class="flex items-center gap-1.5"><span class="size-2.5 rounded-full" :style="{ backgroundColor: CHANGE_COLORS.added }" />{{ totals.added }} {{ $t('queryComparisonAdded') }}</span>
                <span class="text-dimmed">{{ totals.unchanged }} {{ $t('queryComparisonUnchanged') }}</span>
                <UPopover v-if="hasExpectation" mode="hover" :open-delay="100">
                    <button type="button" class="ml-4 flex items-center gap-2 rounded-lg ring ring-inset ring-default px-3 py-1.5 text-default">
                        <span class="flex items-center gap-1.5 text-muted">
                            <span class="size-2.5 rounded-full border-2" :style="{ borderColor: DEVIATION_COLOR }" />
                            {{ $t('queryComparisonExpectation') }}
                        </span>
                        <UBadge v-for="side in accuracySides" :key="side.label" color="neutral" variant="soft" class="rounded-full px-2.5">
                            {{ side.label }} <span class="tabular-nums" :class="side.wrong && 'font-semibold'">{{ side.wrong }}</span> {{ $t('queryComparisonWrong') }}
                        </UBadge>
                    </button>
                    <template #content>
                        <div class="p-3 text-sm">
                            <table class="tabular-nums">
                                <thead>
                                    <tr class="text-muted">
                                        <th />
                                        <th v-for="side in accuracySides" :key="side.label" class="px-3 font-normal text-right">{{ side.label }}</th>
                                    </tr>
                                </thead>
                                <tbody>
                                    <tr v-for="row in accuracyRows" :key="row.key">
                                        <td class="pr-3 text-muted">{{ row.label }}</td>
                                        <td v-for="side in accuracySides" :key="side.label" class="px-3 text-right">{{ side[row.key] }}</td>
                                    </tr>
                                    <tr class="border-t border-default font-semibold">
                                        <td class="pr-3 pt-1">{{ $t('queryComparisonWrongTotal') }}</td>
                                        <td v-for="side in accuracySides" :key="side.label" class="px-3 pt-1 text-right">{{ side.wrong }}</td>
                                    </tr>
                                </tbody>
                            </table>
                            <p class="mt-2 max-w-64 text-dimmed">{{ $t('queryComparisonExpectedNotes', { notes: expectedNoteCount, n: expectedChorales.size }, expectedChorales.size) }}</p>
                        </div>
                    </template>
                </UPopover>
            </div>
            <div class="flex flex-wrap items-center justify-between gap-4 my-4">
                <div class="flex flex-wrap items-center gap-3">
                    <UTooltip :text="$t('queryComparisonUnitFixed')" :disabled="!hasExpectation">
                        <UTabs v-if="notesAvailable || hasExpectation" :model-value="unit" :items="unitItems" :content="false" size="xs" @update:model-value="preferredUnit = $event" />
                    </UTooltip>
                    <UTabs v-model="view" :items="viewItems" :content="false" size="xs" />
                    <UCheckbox v-model="onlyChanges" size="xs" :label="$t('queryComparisonOnlyChanges')" />
                </div>
                <div class="flex flex-wrap items-center justify-end gap-2 ml-auto">
                    <ScoreOptionsPalette :score-options="scoreOptions" size="xs" />
                    <ScorePlayerSettings variant="outline" size="xs" />
                    <UFieldGroup>
                        <USelect v-model="sortKey" :items="sortItems" size="xs" class="w-28" :aria-label="$t('sortBy')" />
                        <UButton
                            color="neutral"
                            variant="outline"
                            size="xs"
                            :icon="sortDescending ? 'lucide:arrow-down-wide-narrow' : 'lucide:arrow-up-narrow-wide'"
                            :aria-label="sortDescending ? $t('sortDescending') : $t('sortAscending')"
                            @click="sortDescending = !sortDescending"
                        />
                    </UFieldGroup>
                    <UPagination v-model:page="page" :total="listedChorales.length" :items-per-page="CHORALES_PER_PAGE" :sibling-count="1" size="xs" />
                </div>
            </div>

            <UEmpty v-if="!listedChorales.length" :title="$t(onlyChanges ? 'queryComparisonNoChanges' : 'noResults')" icon="lucide:equal" />

            <div class="flex flex-col gap-4">
                <UCard v-for="chorale in pagedChorales" :key="chorale.choraleId">
                    <template #header>
                        <div class="flex items-center justify-between gap-4">
                            <NuxtLink :to="localePath(`/chorale/bach/${chorale.choraleId}`)" class="flex flex-wrap items-baseline gap-x-2 hover:text-primary">
                                <span class="font-semibold">{{ chorale.choraleId }}</span>
                                <span>{{ metadata?.[chorale.choraleId]?.title }}</span>
                            </NuxtLink>
                            <div class="flex items-center gap-2">
                                <UBadge color="neutral" variant="outline" :label="`${chorale.unchanged.length} ${$t('queryComparisonUnchanged')}`" />
                                <UBadge v-if="chorale.lost.length" color="error" variant="subtle" :label="`${chorale.lost.length} ${$t('queryComparisonLost')}`" />
                                <UBadge v-if="chorale.added.length" color="success" variant="subtle" :label="`${chorale.added.length} ${$t('queryComparisonAdded')}`" />
                                <UBadge v-if="hasExpectation && deviationCount(chorale)" color="neutral" variant="outline" :label="`${deviationCount(chorale)} ${$t('queryComparisonWrong')}`" />
                                <ScorePlayerControls :player="players.get(chorale.choraleId)" compact />
                            </div>
                        </div>
                    </template>
                    <PlayableHighlightedScore
                        :player="players.get(chorale.choraleId)"
                        :horizontal="scoreOptions.showHorizontalViewMode"
                        :score-url="localScoreUrlGenerator(chorale.choraleId)"
                        :verovio-options="{ ...scoreOptions.verovioOptions, pageMarginLeft: 42 }"
                        :base-filters="scoreOptions.baseFilters"
                        :filters="scoreOptions.humdrumFilters"
                        :scroll-to-first-section="scoreOptions.showHorizontalViewMode"
                        v-bind="highlightGroups(chorale)"
                    />
                </UCard>
            </div>
            <div class="flex justify-center my-4">
                <UPagination v-model:page="page" :total="listedChorales.length" :items-per-page="CHORALES_PER_PAGE" />
            </div>
        </template>
    </UContainer>
</template>
