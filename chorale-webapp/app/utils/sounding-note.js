// The note a voice sounds at a line: the one it attacks there, or else the last one before it that it
// holds (or ties). Null where it has none. `container` is the element the score is drawn in, and the
// ids are verovio's, like "note-L12F3" for the note the third voice attacks on line 12.
export function soundingNoteElem(container, line, voice) {
    const attacked = container?.querySelector(`g#note-L${line}F${voice}`);
    if (attacked) return attacked;

    let held = null;
    let heldLine = -1;
    for (const elem of container?.querySelectorAll('g[id^="note-L"]') ?? []) {
        const match = elem.id.match(/^note-L(\d+)F(\d+)/);
        if (!match || Number(match[2]) !== voice) continue;
        const attackLine = Number(match[1]);
        if (attackLine < line && attackLine > heldLine) {
            held = elem;
            heldLine = attackLine;
        }
    }
    return held;
}
