// The notes and rests of the drawn score by Verovio's ids of them, like "note-L12F3" for the note
// the third voice attacks on line 12 of the score, or "rest-L12F3" and "mrest-L12F3" for a rest.

// The line and the voice (field) of the id of a note or a rest, null for any other id.
export function lineAndVoiceOf(id) {
    const match = id?.match(/^(?:note|rest|mrest)-L(\d+)F(\d+)/);
    return match ? { line: Number(match[1]), voice: Number(match[2]) } : null;
}

// The note a voice sounds at a line: the one it attacks there, or else the last one before it that it
// holds (or ties). Null where it has none. `container` is the element the score is drawn in.
export function soundingNoteElem(container, line, voice) {
    const attacked = container?.querySelector(`g#note-L${line}F${voice}`);
    if (attacked) return attacked;

    let held = null;
    let heldLine = -1;
    for (const elem of container?.querySelectorAll('g[id^="note-L"]') ?? []) {
        const parsed = lineAndVoiceOf(elem.id);
        if (!parsed || parsed.voice !== voice) continue;
        if (parsed.line < line && parsed.line > heldLine) {
            held = elem;
            heldLine = parsed.line;
        }
    }
    return held;
}
