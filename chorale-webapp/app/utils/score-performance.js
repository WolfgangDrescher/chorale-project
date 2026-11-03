// The score as Verovio plays it, in milliseconds of its own tempo: the notes by voice (the highest
// first), the soundings (see soundingsOf), the number of voices, the tempo in quarter notes per
// minute and the duration.
export async function loadScorePerformance(callVerovioMethod) {
    const [midiBase64, timemap, { Midi }] = await Promise.all([
        callVerovioMethod('renderToMIDI'),
        callVerovioMethod('renderToTimemap', { includeMeasures: false, includeRests: true }),
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

    return {
        notes,
        soundings: soundingsOf(timemap),
        voiceCount: tracks.length,
        tempo: midi.header.tempos[0]?.bpm ?? 120,
        duration: Math.max(0, ...notes.map(({ end }) => end)),
    };
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

// The ids of the elements that sound at a time of the performance.
export function idsSoundingAt(performance, time) {
    return performance.soundings.filter(({ start, end }) => start <= time && time < end).map(({ id }) => id);
}
