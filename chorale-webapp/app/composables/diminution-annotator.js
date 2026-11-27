import { useDebounceFn, useTimeoutFn } from '@vueuse/core';

const VOICES = ['bass', 'tenor', 'alto', 'soprano'];

// A note of the file, ["3/2+1/2", "alto"], or a figure, {"voice": "tenor", "preparation": "3/4", ...}, as the
// figures of the queries are: a voice and the lines of the named positions. Uncertain as ["3/2+1/2", "alto", "?"]
// or with "uncertain": true.
function figureOfAnnotation(annotation, lineOf) {
    if (Array.isArray(annotation)) {
        const [position, voiceName, uncertain] = annotation;
        return { voice: VOICES.indexOf(voiceName) + 1, positionLines: { $note: lineOf(position) }, uncertain: uncertain === '?' };
    }
    const { voice, uncertain = false, ...positionsByRole } = annotation;
    const positionLines = Object.fromEntries(Object.entries(positionsByRole).map(([role, position]) => [`$${role}`, lineOf(position)]));
    return { voice: VOICES.indexOf(voice) + 1, positionLines, uncertain };
}

function annotationOfFigure({ voice, positionLines, uncertain }, positionOf) {
    const voiceName = VOICES[voice - 1];
    if (Object.keys(positionLines).length === 1) {
        return [positionOf(Object.values(positionLines)[0]), voiceName, ...(uncertain ? ['?'] : [])];
    }
    const positionsByRole = Object.fromEntries(Object.entries(positionLines).map(([name, line]) => [name.slice(1), positionOf(line)]));
    return { voice: voiceName, ...positionsByRole, ...(uncertain && { uncertain: true }) };
}

const linesOf = ({ positionLines }) => Object.values(positionLines);
const firstLine = (figure) => Math.min(...linesOf(figure));
// The figure a click on a note takes away or makes (un)certain: one of its notes, or of a suspension its
// dissonance, as in a chain of suspensions a resolution is the preparation of the next.
const isClickedBy = (figure, voice, line) => figure.voice === voice
    && ('$suspension' in figure.positionLines ? figure.positionLines.$suspension === line : linesOf(figure).includes(line));

// The diminutions annotated by hand in a chorale, as figures by diminution, loaded once `needed`, and, while
// `active`, annotated with clicks on its notes: a click adds a note to `current` or takes it away, an
// `uncertain` one (Alt) adds it as uncertain or switches between certain and uncertain. A suspension is made of
// three clicks in one voice, its preparation, dissonance and resolution, and clicked on its dissonance after
// that. With saves, as in the development server, the annotations go to bach-diminutions.json.
export function useDiminutionAnnotator(choraleId, { needed, active, saves = false }) {
    const annotator = reactive({
        active,
        // Null until loaded.
        figures: null,
        current: 'consonantLeap',
        // The notes clicked so far for a suspension, each its voice and line.
        pending: [],
        // null, 'saving', 'saved' or 'failed'.
        status: null,
        error: null,
        saves,
        json: computed(() => formatDiminutionAnnotations({ [choraleId]: annotations.value })),
        issueUrl: computed(() => `${REPOSITORY_URL}/issues/new?${new URLSearchParams({ title: `Diminutions of ${choraleId}`, body: `\`\`\`json\n${annotator.json}\`\`\`` })}`),
        click,
        replace,
        clear,
        download,
    });

    let positions = null;
    let lines = null;

    async function load() {
        const score = await $fetch(`/positions/bach-370-chorales/${choraleId}.json`, { cache: 'no-store' });
        positions = new Map(Object.entries(score.positions).map(([line, position]) => [Number(line), position]));
        lines = new Map();
        for (const [line, position] of positions) {
            if (!lines.has(position)) lines.set(position, line);
        }
        const annotated = DIMINUTION_ANNOTATIONS[choraleId] ?? {};
        annotator.figures = Object.fromEntries(DIMINUTIONS.map(({ id }) => [
            id,
            (annotated[id] ?? []).map((annotation) => figureOfAnnotation(annotation, (position) => lines.get(position))),
        ]));
        saved.value = JSON.stringify(annotations.value);
    }

    watch(() => toValue(needed) && !annotator.figures, (shouldLoad) => {
        if (shouldLoad && import.meta.client) load();
    }, { immediate: true });

    watch(() => annotator.current, () => {
        annotator.pending = [];
    });

    // The annotations of the chorale as the file holds them, in the order of the score.
    const annotations = computed(() => Object.fromEntries(Object.entries(annotator.figures ?? {}).map(([id, figures]) => [
        id,
        [...figures].sort((a, b) => firstLine(a) - firstLine(b) || a.voice - b.voice).map((figure) => annotationOfFigure(figure, (line) => positions.get(line))),
    ])));

    function setFigures(id, figures) {
        annotator.figures = { ...annotator.figures, [id]: figures };
    }

    // In place of all annotations, the figures by diminution, e.g. as the queries find them. A suspension
    // stays a figure, of any other diminution every note is one of its own, as the file has them.
    function replace(figuresById) {
        annotator.pending = [];
        annotator.figures = Object.fromEntries(DIMINUTIONS.map(({ id }) => {
            const figures = figuresById[id] ?? [];
            if (id === 'suspension') return [id, figures];
            const notes = new Map(figures.flatMap(({ voice, positionLines }) => Object.values(positionLines).map((line) => [`${voice}:${line}`, { voice, positionLines: { $note: line } }])));
            return [id, [...notes.values()]];
        }));
    }

    function clear() {
        replace({});
    }

    // A click on the note `voice` attacks on `line`, or, `held`, holds there: only the dissonance of a
    // suspension can be a held note.
    function click({ voice, line, held = false, uncertain = false }) {
        if (!annotator.active || !annotator.figures || !positions.has(line)) return;
        const id = annotator.current;
        if (held && id !== 'suspension') return;
        const figures = annotator.figures[id];
        const clicked = figures.find((figure) => isClickedBy(figure, voice, line));
        if (clicked) {
            const next = uncertain ? [{ ...clicked, uncertain: !clicked.uncertain }] : [];
            setFigures(id, figures.flatMap((figure) => (figure === clicked ? next : [figure])));
            return;
        }
        if (id !== 'suspension') {
            setFigures(id, [...figures, { voice, positionLines: { $note: line }, uncertain }]);
            return;
        }
        const pending = annotator.pending.filter((note) => note.voice === voice);
        if (pending.some((note) => note.line === line)) {
            annotator.pending = pending.filter((note) => note.line !== line);
            return;
        }
        annotator.pending = [...pending, { voice, line }].sort((a, b) => a.line - b.line);
        if (annotator.pending.length < 3) return;
        const [preparation, suspension, resolution] = annotator.pending.map((note) => note.line);
        setFigures(id, [...figures, { voice, positionLines: { $preparation: preparation, $suspension: suspension, $resolution: resolution }, uncertain }]);
        annotator.pending = [];
    }

    // Saves the block as chorNNN.json.
    function download() {
        const url = URL.createObjectURL(new Blob([annotator.json], { type: 'application/json' }));
        const link = document.createElement('a');
        link.href = url;
        link.download = `${choraleId}.json`;
        link.click();
        URL.revokeObjectURL(url);
    }

    // "Saved" shows for a moment only.
    const { start: clearSavedSoon } = useTimeoutFn(() => {
        if (annotator.status === 'saved') annotator.status = null;
    }, 3000, { immediate: false });

    async function save() {
        annotator.status = 'saving';
        annotator.error = null;
        const now = JSON.stringify(annotations.value);
        try {
            await $fetch(`/api/dev/diminutions/${choraleId}`, { method: 'PUT', body: { diminutions: annotations.value } });
            saved.value = now;
            annotator.status = 'saved';
            clearSavedSoon();
        } catch (error) {
            annotator.status = 'failed';
            annotator.error = error.data?.message ?? error.message;
        }
    }

    // What the file holds, as far as the annotator knows: loaded or saved last.
    const saved = ref(null);

    const saveSoon = useDebounceFn(save, 800);
    watch(() => JSON.stringify(annotations.value), (now) => {
        if (annotator.saves && saved.value !== null && now !== saved.value) saveSoon();
    });

    return annotator;
}
