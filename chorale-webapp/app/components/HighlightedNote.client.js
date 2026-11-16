// A circle around a note. Where the voice of its id holds a note from an earlier line, the circle is at
// that line, at the height of the held note: when the interval it is part of sounds.
export default defineComponent({
    name: 'HighlightedNote',
    props: {
        noteId: String,
        color: String,
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
            backgroundColor: props.color ?? 'rgb(234 179 8 / 0.4)',
        });

        return () => h('div', {
            style: position,
            class: [
                // props.color,
                'absolute -translate-x-1/2 -translate-y-1/2',
                elem?.classList.contains('verse') ? 'rounded' : 'rounded-full',
            ],
        });
    },
});
