// A line between the noteheads of two notes, for relations between notes rather than for
// stretches of the score: the two fifths of a parallel, say. Drawn from notehead edge to
// notehead edge, over the staff lines and under everything of the two notes themselves (stem,
// accidental, dots, flag): the notes are drawn once more over the line, in their own place.

// A copy of the score's svg that holds nothing but the given note, at the same size and position, so
// that it can be laid over the original: the note's ancestors are copied one by one (without their
// children) for the transforms and the scale they carry. What the note refers to (its glyphs in the
// svg's defs, the style of the score) is found by the document, not by the svg.
function cloneNoteInOwnSvg(noteElem) {
    // The outermost svg: the score is an svg inside an svg (the inner one carries the scale), and
    // the size and position on the page are those of the outer one only.
    let rootSvg = noteElem.closest('svg');
    while (rootSvg.parentElement?.closest('svg')) rootSvg = rootSvg.parentElement.closest('svg');
    const ancestors = [];
    for (let elem = noteElem.parentElement; elem && elem !== rootSvg; elem = elem.parentElement) ancestors.unshift(elem);

    const shallowCopy = (elem) => {
        const copy = elem.cloneNode(false);
        copy.removeAttribute('id');
        return copy;
    };
    const copy = shallowCopy(rootSvg);
    let parent = copy;
    for (const ancestor of ancestors) {
        const ancestorCopy = shallowCopy(ancestor);
        parent.appendChild(ancestorCopy);
        parent = ancestorCopy;
    }
    parent.appendChild(noteElem.cloneNode(true));
    return { copy, rootSvg };
}

export default defineComponent({
    name: 'HighlightedConnection',
    props: {
        from: { type: Object, required: true }, // { line, voice }
        to: { type: Object, required: true },
        color: String,
        width: { type: Number, default: 3 },
        container: HTMLElement,
    },
    setup(props) {
        const noteHead = (position) => {
            const id = `L${position.line}F${position.voice}`;
            const note = props.container.querySelector(`g#note-${id}`);
            return note?.querySelector('.notehead') ?? note;
        };

        const noteOf = (position) => props.container.querySelector(`g#note-L${position.line}F${position.voice}`);
        const noteElems = [noteOf(props.from), noteOf(props.to)];

        const fromElem = noteHead(props.from);
        const toElem = noteHead(props.to);
        if (!fromElem || !toElem) return () => h('div');

        const containerRect = props.container.getBoundingClientRect();
        const center = (elem) => {
            const rect = elem.getBoundingClientRect();
            return { x: rect.x + rect.width / 2 - containerRect.x, y: rect.y + rect.height / 2 - containerRect.y, radius: rect.height / 2 };
        };
        const a = center(fromElem);
        const b = center(toElem);

        // Both ends pulled back by the notehead's radius along the line.
        const length = Math.hypot(b.x - a.x, b.y - a.y);
        if (length === 0) return () => h('div');
        const ux = (b.x - a.x) / length;
        const uy = (b.y - a.y) / length;
        const line = {
            x1: a.x + ux * a.radius,
            y1: a.y + uy * a.radius,
            x2: b.x - ux * b.radius,
            y2: b.y - uy * b.radius,
        };

        const noteLayer = ref(null);
        onMounted(() => {
            for (const noteElem of noteElems) {
                const { copy, rootSvg } = cloneNoteInOwnSvg(noteElem);
                const rootRect = rootSvg.getBoundingClientRect();
                const containerRect = props.container.getBoundingClientRect();
                // The size the original really has on the page, not the one its attributes state: the
                // styles of the page decide it, and they don't reach the copy.
                copy.style.position = 'absolute';
                copy.style.left = `${rootRect.x - containerRect.x}px`;
                copy.style.top = `${rootRect.y - containerRect.y}px`;
                copy.style.width = `${rootRect.width}px`;
                copy.style.height = `${rootRect.height}px`;
                copy.style.maxWidth = 'none';
                copy.style.margin = '0';
                copy.style.pointerEvents = 'none';
                noteLayer.value.appendChild(copy);
            }
        });

        return () => h('div', [
            h(
                'svg',
                { class: 'absolute top-0 left-0 z-10 w-full h-full overflow-visible pointer-events-none' },
                [h('line', { ...line, stroke: props.color, 'stroke-width': props.width, 'stroke-linecap': 'round' })],
            ),
            h('div', { ref: noteLayer, class: 'absolute top-0 left-0 z-20 pointer-events-none' }),
        ]);
    },
});
