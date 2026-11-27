import { NoteChip } from '#components';

// A circle around a note. Where the voice of its id holds a note from an earlier line, the circle is at
// that line, at the height of the held note: when the interval it is part of sounds.
export default defineComponent({
    name: 'HighlightedNote',
    props: {
        noteId: String,
        color: String,
        borderColor: String,
        // Chips with an icon or a text on the edge of the circle, each { icon, text, color, angle }, the angle in
        // degrees clockwise from the top.
        chips: { type: Array, default: () => [] },
        container: HTMLElement,
    },
    setup(props) {
        const note = lineAndVoiceOf(`note-${props.noteId}`);
        const field = note && fieldOfVoice(props.container, note.voice);
        const id = note ? `L${note.line}F${field}` : props.noteId;
        const selector = `g#note-${id}, g#rest-${id}, g#mrest-${id}, g#verse-${props.noteId}`;
        const noteHead = (noteElem) => noteElem?.querySelector('.notehead') || noteElem;
        let elem = props.container.querySelector(selector);
        let atLine = null;
        if (!elem) {
            elem = note && soundingNoteElem(props.container, note.line, field);
            atLine = note && props.container.querySelector(`g[id^="note-L${note.line}F"]`);
            // A note held across a system break has no height on the system of the line.
            if (atLine?.closest('.system') !== elem?.closest('.system')) atLine = null;
        }

        if (!elem) return () => h('div');

        elem = noteHead(elem);

        const elemRect = elem.getBoundingClientRect();
        const xRect = atLine ? noteHead(atLine).getBoundingClientRect() : elemRect;
        const parentRect = props.container.getBoundingClientRect();
        // Three times the notehead, at any scale.
        const markerSize = elemRect.height * 3;
        const widthExtender = 10;
        const width = elem.classList.contains('verse') ? elemRect.width + widthExtender : markerSize;

        const position = reactive({
            width: `${width}px`,
            height: `${markerSize}px`,
            left: `${xRect.x + xRect.width / 2 - parentRect.x}px`,
            top: `${elemRect.y + elemRect.height / 2 - parentRect.y}px`,
            background: props.color ?? 'rgb(234 179 8 / 0.4)',
            ...(props.borderColor && { border: `2px solid ${props.borderColor}` }),
        });

        // Whole and even pixels: a fractional size rounds the chip to an ellipse, an odd one leaves its icon off
        // center.
        const chipSize = Math.max(2, Math.round((markerSize * 0.52) / 2) * 2);
        // In percent of the circle, a little outside its edge.
        const chipRadius = 56;
        // Where a chip of a longer text starts its pill: at the circle a chip of one character would fill, and
        // from there to the right on the right half of the note, to the left on its left half.
        const pillTranslate = (angle) => ((angle % 360 + 360) % 360 < 180 ? `${-chipSize / 2}px -50%` : `calc(${chipSize / 2}px - 100%) -50%`);
        const chip = ({ icon, text, color, angle }) => h(NoteChip, {
            icon,
            text,
            color,
            size: chipSize,
            class: 'absolute ring-1 ring-white',
            style: {
                left: `${50 + chipRadius * Math.sin((angle * Math.PI) / 180)}%`,
                top: `${50 - chipRadius * Math.cos((angle * Math.PI) / 180)}%`,
                translate: icon || text.length === 1 ? '-50% -50%' : pillTranslate(angle),
            },
        });

        return () => h('div', {
            style: position,
            class: [
                // props.color,
                'absolute -translate-x-1/2 -translate-y-1/2',
                elem?.classList.contains('verse') ? 'rounded' : 'rounded-full',
            ],
        }, props.chips.map(chip));
    },
});
