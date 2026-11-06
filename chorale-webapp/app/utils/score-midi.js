// General MIDI programs of the instruments of the player, counted from 0.
const MIDI_PROGRAMS = { church_organ: 19, choir_aahs: 52, voice_oohs: 53, grand_piano: 0 };
const PAN_CONTROLLER = 10;
const REVERB_CONTROLLER = 91;
// Channel 10 (9 from 0) is for drums.
const channelOf = (track) => (track < 9 ? track : track + 1);

// The performance as a MIDI file, in the tempo and mix of the player settings, without the muted
// voices. The volume of a voice scales its velocities, so an acoustic instrument like a Disklavier
// follows it, too.
export async function performanceToMidi(performance, { instrument, tempo, reverb, voices }) {
    const { Midi } = await import('@tonejs/midi');
    const midi = new Midi();
    midi.header.setTempo(tempo);
    // The performance is in milliseconds of the tempo of the score.
    const ticksOf = (time) => Math.round(time / 60000 * performance.tempo * midi.header.ppq);

    voicesByTrack(performance.voiceCount).forEach((voice, index) => {
        const { isMuted, volume, pan, reverb: voiceReverb } = voices[voice];
        if (isMuted) return;
        const track = midi.addTrack();
        track.name = voice;
        track.channel = channelOf(index);
        track.instrument.number = MIDI_PROGRAMS[instrument];
        // Control values from 0 to 1.
        track.addCC({ number: PAN_CONTROLLER, value: (pan + 100) / 200, ticks: 0 });
        track.addCC({ number: REVERB_CONTROLLER, value: Math.min(reverb / 100 * voiceReverb / 100, 1), ticks: 0 });
        for (const note of performance.notes) {
            if (note.voice !== index) continue;
            const ticks = ticksOf(note.start);
            track.addNote({
                midi: note.pitch,
                ticks,
                durationTicks: ticksOf(note.end) - ticks,
                velocity: note.velocity / 127 * volume / 100,
            });
        }
    });
    return midi.toArray();
}
