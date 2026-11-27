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
// figures as their voice and where each of their notes sounds by its role. Uncertain with a "?" after the voice of a
// note, with "uncertain": true in a figure.
export const DIMINUTION_ANNOTATIONS = annotations;

// The question mark of a note annotated as uncertain, where no diminution gives it a color.
export const UNCERTAIN_COLOR = 'rgb(100 116 139)';

const opaque = (color) => color.replace(/ \/ [\d.]+\)$/, ')');

// Where the chip of the share `index` of a circle of `count` colors sits on its edge: above that share where it
// can, the upper right for a single color.
function chipAngle(index, count) {
    const start = (360 * index) / count;
    const end = (360 * (index + 1)) / count;
    return [45, 315, 135, 225].find((angle) => angle > start && angle < end) ?? (start + end) / 2;
}

// The annotator of the development server saves to the file while it annotates, which would reload the page.
if (import.meta.hot) {
    import.meta.hot.accept('../../../annotations/bach-diminutions.json', (module) => {
        for (const choraleId of Object.keys(DIMINUTION_ANNOTATIONS)) delete DIMINUTION_ANNOTATIONS[choraleId];
        Object.assign(DIMINUTION_ANNOTATIONS, module.default);
    });
}

const VOICES = ['bass', 'tenor', 'alto', 'soprano'];

// The notes annotated for a diminution, by chorale, each as its voice, line and whether it is uncertain, which
// it is only where every annotation of it is. Empty until loaded.
export function useExpectedDiminutions(diminutionId) {
    const chorales = shallowRef(new Map());

    async function loadChorale(choraleId, annotationsOfDiminution) {
        const { positions } = await $fetch(`/positions/bach-370-chorales/${choraleId}.json`);
        // The first line of a position, as the tests of chorale-search read it.
        const lines = Object.fromEntries(Object.entries(positions).reverse().map(([line, position]) => [position, Number(line)]));
        const notes = annotationsOfDiminution.flatMap((annotation) => {
            if (Array.isArray(annotation)) return [{ position: annotation[0], voiceName: annotation[1], uncertain: annotation[2] === '?' }];
            const { voice, uncertain = false, ...positionsByRole } = annotation;
            return Object.values(positionsByRole).map((position) => ({ position, voiceName: voice, uncertain }));
        });
        const byKey = new Map();
        for (const { position, voiceName, uncertain } of notes) {
            const note = { voice: VOICES.indexOf(voiceName) + 1, line: lines[position], uncertain };
            const key = `${note.voice}:${note.line}`;
            byKey.set(key, { ...note, uncertain: uncertain && (byKey.get(key)?.uncertain ?? true) });
        }
        return [choraleId, [...byKey.values()]];
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

    // The notes of the figures of each diminution, by id, e.g. "L52F3", and whether they are uncertain.
    const notesById = computed(() => Object.fromEntries(DIMINUTIONS.map(({ id }) => [
        id,
        (figures.value[id] ?? []).flatMap(({ voice, positionLines, uncertain = false }) => Object.values(positionLines).map((line) => ({ noteId: `L${line}F${voice}`, uncertain }))),
    ])));

    const legend = computed(() => DIMINUTIONS.map((diminution) => {
        const figuresOfDiminution = figures.value[diminution.id] ?? [];
        const uncertainCount = figuresOfDiminution.filter(({ uncertain }) => uncertain).length;
        return {
            ...diminution,
            shown: toValue(shownIds).includes(diminution.id),
            count: figuresOfDiminution.length - uncertainCount,
            uncertainCount,
        };
    }));

    const shown = computed(() => legend.value.filter((diminution) => diminution.shown));

    // A note of several diminutions is a circle divided between their colors, with a question mark on the edge of
    // each uncertain share. An uncertain suspension has its question mark at its dissonance.
    const notes = computed(() => {
        const sharesByNote = new Map();
        for (const { id, color } of shown.value) {
            if (id === 'suspension') continue;
            for (const { noteId, uncertain } of notesById.value[id]) {
                sharesByNote.set(noteId, [...(sharesByNote.get(noteId) ?? []), { color: highlightColorsByName[color], uncertain }]);
            }
        }
        const groups = new Map();
        for (const [noteId, shares] of sharesByNote) {
            const fill = shares.length === 1 ? shares[0].color : `conic-gradient(${shares.map(({ color }, index) => `${color} 0 ${(100 * (index + 1)) / shares.length}%`).join(', ')})`;
            const chips = shares.flatMap(({ color, uncertain }, index) => (uncertain ? [{ icon: 'lucide:question-mark', color: opaque(color), angle: chipAngle(index, shares.length) }] : []));
            const key = `${fill}:${JSON.stringify(chips)}`;
            const group = groups.get(key) ?? { color: fill, chips, items: [] };
            groups.set(key, { ...group, items: [...group.items, noteId] });
        }
        const suspensionShown = shown.value.some(({ id }) => id === 'suspension');
        const uncertainDissonances = (suspensionShown ? figures.value.suspension ?? [] : [])
            .filter(({ uncertain }) => uncertain)
            .map(({ voice, positionLines }) => `L${positionLines.$suspension}F${voice}`);
        // The notes clicked for a suspension that is not complete yet.
        const pending = annotator.pending.map(({ voice, line }) => `L${line}F${voice}`);
        return [
            ...groups.values(),
            { color: 'transparent', chips: [{ icon: 'lucide:question-mark', color: opaque(highlightColorsByName.blue), angle: 45 }], items: uncertainDissonances },
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
