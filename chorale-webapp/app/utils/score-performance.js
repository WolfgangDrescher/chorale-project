// The score as Verovio plays it, in milliseconds of its own tempo: the notes by voice (the highest
// first), the soundings (see soundingsOf), the number of voices, the tempo in quarter notes per
// minute and the duration, with a breath after each fermata.
export async function loadScorePerformance(callVerovioMethod) {
    const [midiBase64, timemap, mei, { Midi }] = await Promise.all([
        callVerovioMethod('renderToMIDI'),
        callVerovioMethod('renderToTimemap', { includeMeasures: false, includeRests: true }),
        callVerovioMethod('getMEI'),
        import('@tonejs/midi'),
    ]);

    const midi = new Midi(Uint8Array.from(atob(midiBase64), (char) => char.charCodeAt(0)));
    const tracks = midi.tracks.filter((track) => track.notes.length);
    const notes = tracks
        .flatMap((track, voice) => track.notes.map((note) => ({
            voice,
            pitch: note.midi,
            // As MIDI counts it, from 0 to 127, which smplr takes; @tonejs/midi gives it from 0 to 1.
            velocity: Math.round(note.velocity * 127),
            start: note.time * 1000,
            end: (note.time + note.duration) * 1000,
        })))
        .sort((a, b) => a.start - b.start);

    return holdFermatas({
        notes,
        soundings: soundingsOf(timemap),
        voiceCount: tracks.length,
        tempo: midi.header.tempos[0]?.bpm ?? 120,
        duration: Math.max(0, ...notes.map(({ end }) => end)),
    }, fermataNoteIds(mei));
}

// A sounding is one time an element of the drawn score sounds, a note or a rest, from the time it
// is attacked to the time it stops, with its id.
function soundingsOf(timemap) {
    const soundings = [];
    const open = new Map();
    for (const { tstamp, on = [], off = [], restsOn = [], restsOff = [] } of timemap) {
        for (const id of [...off, ...restsOff]) {
            const sounding = open.get(id);
            if (sounding) sounding.end = tstamp;
            open.delete(id);
        }
        for (const id of [...on, ...restsOn]) {
            const sounding = { id, start: tstamp, end: Infinity };
            soundings.push(sounding);
            open.set(id, sounding);
        }
    }
    return soundings;
}

function fermataNoteIds(mei) {
    return new Set(Array.from(mei.matchAll(/<fermata\b[^>]*\bstartid="#([^"]+)"/g), (match) => match[1]));
}

// A note with a fermata sounds as long as it is written; before the music goes on, a breath of a
// sixteenth is put in. In quarter notes, so it follows the tempo.
const FERMATA_BREATH = 0.25;

// The ends of the fermatas are read off the timemap, the notes off the MIDI, and the two do not
// agree to the millisecond: Verovio ends a note of the MIDI about 5 ms before the timemap does, so
// that repeated notes stay apart. Times this close are taken as the same moment, e.g. the end of a
// fermata and the end of the notes that end with it.
const SAME_TIME_MS = 20;

// Puts a breath after every fermata the music goes on right after, in all voices at once: the
// notes that end with the fermata sound until the breath, those that begin after it begin after.
function holdFermatas(performance, fermataIds) {
    const breath = FERMATA_BREATH * 60000 / performance.tempo;
    const goesOnAt = (time) => performance.notes.some(({ start }) => Math.abs(start - time) < SAME_TIME_MS);

    // Where the fermatas end, merged where they overlap.
    const ends = [];
    const fermatas = performance.soundings.filter(({ id, end }) => fermataIds.has(id) && end < Infinity);
    for (const { start, end } of fermatas.sort((a, b) => a.start - b.start)) {
        if (ends.length && start <= ends.at(-1)) ends[ends.length - 1] = Math.max(ends.at(-1), end);
        else ends.push(end);
    }
    const breaths = ends.filter(goesOnAt);
    if (!breaths.length) return performance;

    const shiftStart = (time) => time + breath * breaths.filter((end) => end <= time + SAME_TIME_MS).length;
    const shiftEnd = (time) => time + breath * breaths.filter((end) => end < time - SAME_TIME_MS).length;

    return {
        ...performance,
        notes: performance.notes.map((note) => ({ ...note, start: shiftStart(note.start), end: shiftEnd(note.end) })),
        // The notes of the fermata are marked through the breath, until the music goes on.
        soundings: performance.soundings.map((sounding) => ({ ...sounding, start: shiftStart(sounding.start), end: shiftStart(sounding.end) })),
        duration: shiftEnd(performance.duration),
    };
}

// The ids of the elements that sound at a time of the performance.
export function idsSoundingAt(performance, time) {
    return performance.soundings.filter(({ start, end }) => start <= time && time < end).map(({ id }) => id);
}
