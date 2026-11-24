import annotations from '../../../annotations/bach-diminutions.json';
import queries from '../../../chorale-search/queries/diminutions.json';

// The diminutions of the analysis layer. Every position their queries name ("as": "$note") is a
// note marked in the color of the diminution; the tests of chorale-search check the queries.
export const DIMINUTIONS = [
    { id: 'consonantLeap', color: 'green' },
    { id: 'suspension', color: 'blue' },
    { id: 'neighborNote', color: 'orange' },
    { id: 'passingNote', color: 'amber' },
    { id: 'accentedPassingNote', color: 'red' },
    { id: 'anticipation', color: 'yellow' },
    { id: 'leapingNeighborNote', color: 'purple' },
].map((diminution) => ({ ...diminution, queries: queries[diminution.id] }));

// By chorale and diminution, what is annotated by hand: notes as "measure/beat" and voice where they are attacked, or
// figures as their voice and where each of their notes sounds by its role.
export const DIMINUTION_ANNOTATIONS = annotations;

// The annotator of the development server saves to the file while it annotates, which would reload the page.
if (import.meta.hot) {
    import.meta.hot.accept('../../../annotations/bach-diminutions.json', (module) => {
        for (const choraleId of Object.keys(DIMINUTION_ANNOTATIONS)) delete DIMINUTION_ANNOTATIONS[choraleId];
        Object.assign(DIMINUTION_ANNOTATIONS, module.default);
    });
}

const VOICES = ['bass', 'tenor', 'alto', 'soprano'];

// The notes annotated for a diminution, by chorale, each as its voice and line. Empty until loaded.
export function useExpectedDiminutions(diminutionId) {
    const chorales = shallowRef(new Map());

    async function loadChorale(choraleId, annotationsOfDiminution) {
        const { positions } = await $fetch(`/positions/bach-370-chorales/${choraleId}.json`);
        // The first line of a position, as the tests of chorale-search read it.
        const lines = Object.fromEntries(Object.entries(positions).reverse().map(([line, position]) => [position, Number(line)]));
        const notes = annotationsOfDiminution.flatMap((annotation) => {
            if (Array.isArray(annotation)) return [annotation];
            const { voice, ...positionsByRole } = annotation;
            return Object.values(positionsByRole).map((position) => [position, voice]);
        });
        return [choraleId, notes.map(([position, voiceName]) => ({ voice: VOICES.indexOf(voiceName) + 1, line: lines[position] }))];
    }

    watch(() => toValue(diminutionId), async (id) => {
        chorales.value = new Map();
        if (import.meta.server) return;
        const annotated = Object.entries(DIMINUTION_ANNOTATIONS).filter(([, diminutions]) => diminutions[id]);
        const loaded = await Promise.all(annotated.map(([choraleId, diminutions]) => loadChorale(choraleId, diminutions[id])));
        if (toValue(diminutionId) === id) chorales.value = new Map(loaded);
    }, { immediate: true });

    return chorales;
}

// The notes of the diminutions of a chorale as the queries find them, searched once the first of them is
// shown, or, with `source` "annotated", as annotated by hand. While `annotating`, they are annotated as
// useDiminutionAnnotator does.
export function useDiminutionAnalysis(choraleId, shownIds, { source = 'found', annotating = false } = {}) {
    const results = ref(null);

    async function fetchResults() {
        const body = DIMINUTIONS.flatMap(({ id, queries }) => queries.map((query) => ({ id, ...query, choraleFilter: { ids: choraleId } })));
        const response = await $fetch('/api/chorale-search', { method: 'POST', body });
        results.value = Object.values(response.results).flat();
    }

    watch(() => toValue(shownIds).length > 0, (shown) => {
        if (shown && !results.value) fetchResults();
    }, { immediate: true });

    // Each diminution as its figures, each the voice and the lines of its named positions, once however many
    // queries find it.
    const found = computed(() => {
        const byKey = Object.fromEntries(DIMINUTIONS.map(({ id }) => [id, new Map()]));
        for (const { queryId, voice, positionLines = {} } of results.value ?? []) {
            byKey[queryId].set(`${voice}:${JSON.stringify(positionLines)}`, { voice, positionLines });
        }
        return Object.fromEntries(Object.entries(byKey).map(([id, figuresByKey]) => [id, [...figuresByKey.values()]]));
    });

    const annotator = useDiminutionAnnotator(choraleId, {
        needed: () => toValue(source) === 'annotated' || toValue(annotating),
        active: annotating,
        saves: import.meta.dev,
    });

    const showsAnnotated = computed(() => toValue(source) === 'annotated' || toValue(annotating));
    const figures = computed(() => (showsAnnotated.value ? annotator.figures ?? {} : found.value));

    // The ids of the notes of the figures of each diminution, e.g. "L52F3".
    const noteIds = computed(() => Object.fromEntries(DIMINUTIONS.map(({ id }) => [
        id,
        new Set((figures.value[id] ?? []).flatMap(({ voice, positionLines }) => Object.values(positionLines).map((line) => `L${line}F${voice}`))),
    ])));

    const legend = computed(() => DIMINUTIONS.map((diminution) => ({
        ...diminution,
        shown: toValue(shownIds).includes(diminution.id),
        count: figures.value[diminution.id]?.length ?? 0,
    })));

    const shown = computed(() => legend.value.filter((diminution) => diminution.shown));

    // A note of several diminutions is a circle divided between their colors.
    const notes = computed(() => {
        const colorsByNote = new Map();
        for (const { id, color } of shown.value) {
            if (id === 'suspension') continue;
            for (const noteId of noteIds.value[id]) colorsByNote.set(noteId, [...(colorsByNote.get(noteId) ?? []), highlightColorsByName[color]]);
        }
        const notesByFill = new Map();
        for (const [noteId, colors] of colorsByNote) {
            const fill = colors.length === 1 ? colors[0] : `conic-gradient(${colors.map((color, index) => `${color} 0 ${(100 * (index + 1)) / colors.length}%`).join(', ')})`;
            notesByFill.set(fill, [...(notesByFill.get(fill) ?? []), noteId]);
        }
        // The notes clicked for a suspension that is not complete yet.
        const pending = annotator.pending.map(({ voice, line }) => `L${line}F${voice}`);
        return [
            ...[...notesByFill].map(([color, items]) => ({ color, items })),
            { color: highlightColorsByName.blue, borderColor: 'rgb(59 130 246)', items: pending },
        ].filter(({ items }) => items.length);
    });

    // A suspension is a section over its notes, the preparation and the resolution hatched around the dissonance.
    // Held without a tie, the dissonance is the note of the preparation.
    const sections = computed(() => {
        if (!shown.value.some(({ id }) => id === 'suspension')) return [];
        const items = (figures.value.suspension ?? []).map(({ voice, positionLines: { $preparation, $suspension, $resolution } }) => ({
            voice,
            startLine: $preparation,
            endLine: $resolution,
            gaps: [{ startLine: $preparation, endLine: $suspension - 1 }, { startLine: $resolution, endLine: $resolution }],
        }));
        return [{ color: highlightColorsByName.blue, noteheads: true, items }];
    });

    // The figures the queries find in place of all annotations.
    async function annotateFound() {
        if (!results.value) await fetchResults();
        annotator.replace(found.value);
    }

    return { notes, sections, legend, annotator, annotateFound };
}
