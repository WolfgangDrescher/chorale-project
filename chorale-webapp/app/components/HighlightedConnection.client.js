// A line between the noteheads of two notes, for relations between notes rather than for
// stretches of the score: the two fifths of a parallel, say. Drawn from notehead edge to
// notehead edge so neither head is covered.
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

        return () => h(
            'svg',
            { class: 'absolute top-0 left-0 w-full h-full overflow-visible pointer-events-none' },
            [h('line', { ...line, stroke: props.color, 'stroke-width': props.width, 'stroke-linecap': 'round' })],
        );
    },
});
