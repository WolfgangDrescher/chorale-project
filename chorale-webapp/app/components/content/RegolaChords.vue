<script setup>
const DECKS = ['core', 'extended'];
const TAGS = REGOLA_TAGS.filter((tag) => !DECKS.includes(tag));
const MODES = { both: 'Both', major: 'Major', minor: 'Minor' };
const DEGREE_ORDER = ['1', '2', '3', '4', '4+', '5', '6', '7'];
// Drawn as icons: the text font has no arrows, and those of the fonts it falls back to differ from each other.
const MOTIONS = { ascending: { icon: 'lucide:arrow-up-right', title: 'up by step' }, descending: { icon: 'lucide:arrow-down-right', title: 'down by step' }, leap: { text: 'leap' } };

// The red and blue of the two decks of Kartimento.
const DECK_CHIP = { core: 'ring-rose-300 bg-rose-50 text-rose-900 hover:bg-rose-100', extended: 'ring-sky-300 bg-sky-50 text-sky-900 hover:bg-sky-100' };
const DECK_CHIP_INCLUDED = { core: 'ring-rose-500 bg-rose-500 text-white hover:bg-rose-600', extended: 'ring-sky-600 bg-sky-600 text-white hover:bg-sky-700' };
const DECK_TAG = { core: 'border-rose-300 bg-rose-50 text-rose-900', extended: 'border-sky-300 bg-sky-50 text-sky-900' };
const DECK_TAG_INCLUDED = { core: 'border-rose-500 bg-rose-500 text-white', extended: 'border-sky-600 bg-sky-600 text-white' };
const DECK_ROW = { core: 'border-l-4 border-l-rose-300', extended: 'border-l-4 border-l-sky-300' };

const COLUMNS = [
    { id: 'bass', header: 'Bass' },
    { id: 'chord', header: 'Chord', meta: { class: { td: 'text-2xl text-default' } } },
    { id: 'context', header: 'Context', meta: { class: { td: 'whitespace-normal' } } },
    { id: 'tags', header: 'Tags', meta: { class: { td: 'whitespace-normal' } } },
];
const TABLE_UI = {
    root: 'rounded-md border border-default',
    thead: 'bg-elevated/60',
    th: 'px-3 py-1.5 text-xs font-medium text-muted',
    td: 'px-3 py-1',
};
const tableMeta = { class: { tr: (row) => DECK_ROW[row.original.deck] } };

// A click includes a tag, a second one excludes it with "!" as in regolaTags, a third one clears it.
const NEXT_STATE = { include: 'exclude', exclude: null };
const DEFAULT_SELECTION = { core: 'include' };
const selection = ref(DEFAULT_SELECTION);
const mode = ref('both');

const included = computed(() => Object.keys(selection.value).filter((tag) => selection.value[tag] === 'include'));
const excluded = computed(() => Object.keys(selection.value).filter((tag) => selection.value[tag] === 'exclude'));
const regolaTags = computed(() => [...included.value, ...excluded.value.map((tag) => `!${tag}`)]);

const allowsMode = (card) => mode.value === 'both' || !card.mode || card.mode === mode.value;

// As allowedRegolaChords in RegolaChords.cpp: one of the included tags, none of the excluded.
function isAllowed(card) {
    const chosen = !included.value.length || included.value.some((tag) => card.tags.includes(tag));
    return chosen && !excluded.value.some((tag) => card.tags.includes(tag));
}

const counts = computed(() => Object.fromEntries(REGOLA_TAGS.map((tag) => [tag, REGOLA_CHORDS.filter((card) => allowsMode(card) && card.tags.includes(tag)).length])));

const rows = computed(() => REGOLA_CHORDS
    .map((card, index) => ({ ...card, index, deck: card.tags.includes('core') ? 'core' : 'extended' }))
    .filter((card) => allowsMode(card) && isAllowed(card))
    .toSorted((a, b) => DEGREE_ORDER.indexOf(a.scaleDegree) - DEGREE_ORDER.indexOf(b.scaleDegree) || a.index - b.index));

// The default of regolaTags in both modes.
const isDefault = computed(() => mode.value === 'both' && regolaTags.value.join() === 'core');

function reset() {
    selection.value = DEFAULT_SELECTION;
    mode.value = 'both';
}

function cycle(tag) {
    const { [tag]: state, ...others } = selection.value;
    const next = state ? NEXT_STATE[state] : 'include';
    selection.value = next ? { ...others, [tag]: next } : others;
}

function chipClass(tag) {
    const state = selection.value[tag];
    return [
        state === 'exclude' && 'line-through decoration-2',
        DECK_CHIP[tag] && (state === 'include' ? DECK_CHIP_INCLUDED[tag] : DECK_CHIP[tag]),
    ];
}

function tagClass(tag) {
    const isIncluded = selection.value[tag] === 'include';
    if (DECK_TAG[tag]) return isIncluded ? DECK_TAG_INCLUDED[tag] : DECK_TAG[tag];
    return isIncluded ? 'border-inverted bg-inverted text-inverted' : 'border-default bg-elevated text-default';
}

function context(card) {
    return [card.mode && `${card.mode} only`, card.context].filter(Boolean).join(', ');
}
</script>

<template>
    <div class="not-prose my-6 text-sm">
        <div class="flex flex-wrap items-center gap-1.5">
            <template v-for="group in [DECKS, TAGS]" :key="group[0]">
                <span v-if="group === TAGS" class="mx-1 h-5 border-l border-default" />
                <UButton
                    v-for="tag in group"
                    :key="tag"
                    size="xs"
                    color="neutral"
                    :variant="selection[tag] === 'include' ? 'solid' : 'outline'"
                    class="font-mono"
                    :class="chipClass(tag)"
                    @click="cycle(tag)"
                >
                    {{ selection[tag] === 'exclude' ? `!${tag}` : tag }}<span class="opacity-60">{{ counts[tag] }}</span>
                </UButton>
            </template>
        </div>
        <div class="mt-3 flex flex-wrap items-center gap-x-3 gap-y-2">
            <span class="text-muted">
                <b class="text-default">{{ rows.length }}</b> {{ rows.length === 1 ? 'chord' : 'chords' }}<template v-if="mode !== 'both'"> in {{ mode }}</template><template v-if="regolaTags.length"> for <code class="font-mono text-xs text-default">"regolaTags": [{{ regolaTags.map((tag) => `"${tag}"`).join(', ') }}]</code></template>
            </span>
            <UButton v-if="!isDefault" size="xs" color="neutral" variant="link" icon="lucide:rotate-ccw" class="ml-auto" @click="reset">Reset</UButton>
            <UFieldGroup size="xs" :class="isDefault && 'ml-auto'">
                <UButton
                    v-for="(label, option) in MODES"
                    :key="option"
                    color="neutral"
                    :variant="option === mode ? 'solid' : 'outline'"
                    @click="mode = option"
                >
                    {{ label }}
                </UButton>
            </UFieldGroup>
        </div>
        <UTable :data="rows" :columns="COLUMNS" :meta="tableMeta" :ui="TABLE_UI" class="mt-3">
            <template #bass-cell="{ row }">
                <span class="inline-flex items-center gap-1.5">
                    <Deg :v="row.original.scaleDegree" bass />
                    <template v-for="motion in row.original.bassMotions" :key="motion">
                        <UIcon v-if="MOTIONS[motion].icon" :name="MOTIONS[motion].icon" :title="MOTIONS[motion].title" class="size-4" />
                        <span v-else class="text-xs">{{ MOTIONS[motion].text }}</span>
                    </template>
                </span>
            </template>
            <template #chord-cell="{ row }">
                <Fb :v="row.original.figuredBass" literal uniform />
            </template>
            <template #context-cell="{ row }">
                {{ context(row.original) }}
            </template>
            <template #tags-cell="{ row }">
                <span class="flex flex-wrap gap-1">
                    <code v-for="tag in row.original.tags" :key="tag" class="rounded border px-1 font-mono text-xs whitespace-nowrap" :class="tagClass(tag)">{{ tag }}</code>
                </span>
            </template>
        </UTable>
    </div>
</template>
