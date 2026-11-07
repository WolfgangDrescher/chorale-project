import { defineStore, acceptHMRUpdate, skipHydrate } from 'pinia';
import { useLocalStorage } from '@vueuse/core';

// The instruments a score can be played with, all played by smplr: General MIDI instruments of the
// MusyngKite soundfont and the Splendid Grand Piano, loaded the first time they are played. The
// soundfont is not looped, its loop points click; a sample lasts three seconds, enough for a chorale.
export const SCORE_PLAYER_INSTRUMENTS = [
    { id: 'church_organ', label: 'playerInstrumentChurchOrgan' },
    { id: 'choir_aahs', label: 'playerInstrumentChoirAahs' },
    { id: 'voice_oohs', label: 'playerInstrumentVoiceOohs' },
    { id: 'grand_piano', label: 'playerInstrumentGrandPiano' },
];

const defaultVoiceSettings = () => ({
    isMuted: false,
    // In percent.
    volume: 100,
    // From -100 (left) to 100 (right).
    pan: 0,
    // In percent of the reverb of all voices.
    reverb: 100,
    // The stops drawn on the organ, see ORGAN_STOPS.
    organStops: [8],
});

// From the lowest voice on the left to the highest on the right, as at a keyboard.
export const STEREO_SPREAD_PANS = { soprano: 70, alto: 20, tenor: -20, bass: -70 };

const defaultSettings = () => ({
    instrument: 'church_organ',
    // In quarter notes per minute.
    tempo: 72,
    // In percent.
    volume: 80,
    reverb: 20,
    voices: Object.fromEntries(VOICES.map((voice) => [voice, defaultVoiceSettings()])),
});

// The stops of the organ known to this version of the player; at least one stays drawn.
function validOrganStops(stops) {
    const valid = (Array.isArray(stops) ? stops : []).filter((stop) => stop in ORGAN_STOPS);
    return valid.length ? valid : defaultVoiceSettings().organStops;
}

// Shared by all players, so that a score of the search results sounds like the chorale it links
// to, and kept in the local storage. The refs are not hydrated from the server, which has no storage.
export const useScorePlayerSettings = defineStore('score_player_settings', () => {
    const settings = useLocalStorage('chorale-player-settings', defaultSettings(), { mergeDefaults: true });
    // An instrument stored by an earlier version of the player that this one does not have.
    if (!SCORE_PLAYER_INSTRUMENTS.some(({ id }) => id === settings.value.instrument)) settings.value.instrument = defaultSettings().instrument;
    const storedVoices = settings.value.voices ?? {};
    settings.value.voices = Object.fromEntries(VOICES.map((voice) => {
        const stored = { ...defaultVoiceSettings(), ...storedVoices[voice] };
        // Stored for all voices by an earlier version of the player.
        stored.organStops = validOrganStops(storedVoices[voice]?.organStops ?? settings.value.organStops);
        return [voice, stored];
    }));
    delete settings.value.organStops;
    // Stored by an earlier version of the player, which spread the voices with a switch.
    if (settings.value.spreadVoices) for (const voice of VOICES) settings.value.voices[voice].pan = STEREO_SPREAD_PANS[voice];
    delete settings.value.spreadVoices;
    // In place, the refs below point into the object.
    function reset() {
        Object.assign(settings.value, defaultSettings());
    }
    return {
        ...Object.fromEntries(Object.entries(toRefs(settings.value)).map(([key, value]) => [key, skipHydrate(value)])),
        reset,
    };
});

// The stops of the organ by their pitch in feet: the sample of the organ played that many semitones
// away (8' as written, 16' an octave lower, 4' and 2' one and two octaves higher), at that share of
// the velocity of the note, so the 8' carries and the others color it.
export const ORGAN_STOPS = {
    16: { transposition: -12, level: 0.8 },
    8: { transposition: 0, level: 1 },
    4: { transposition: 12, level: 0.7 },
    2: { transposition: 24, level: 0.5 },
};

// The layers each note of a voice is played in: one, or one per stop drawn on the organ. A single
// stop sounds at full level.
function layersPerNote(instrument, organStops) {
    if (instrument !== 'church_organ') return [ORGAN_STOPS[8]];
    if (organStops.length === 1) return [{ ...ORGAN_STOPS[organStops[0]], level: 1 }];
    return organStops.map((stop) => ORGAN_STOPS[stop]);
}

// Notes are handed to the audio clock this far ahead, so the slowed timers of a background tab keep up.
const LOOKAHEAD_SECONDS = 3;
const SCHEDULE_INTERVAL_MS = 500;

// Attacks this close to the position are taken as the one it is at, not as the next or the previous.
const SAME_ATTACK_MS = 5;

// Made and started on the first play, within the click, as browsers require for sound.
let audioContext = null;
let audio = null;

// The gain of the volume of the settings, squared so the slider feels even.
const gainOf = (volume) => (volume / 100) ** 2;

function useAudio() {
    if (!audioContext) {
        audioContext = new AudioContext();
        const master = audioContext.createGain();
        master.connect(audioContext.destination);
        audio = import('smplr').then((smplr) => ({
            smplr,
            context: audioContext,
            master,
            reverb: smplr.Reverb(audioContext),
            loader: smplr.SampleLoader(audioContext),
            // Hands a note due within the lookahead straight to the audio clock, not to a timer.
            scheduler: smplr.Scheduler(audioContext, { lookaheadMs: (LOOKAHEAD_SECONDS + 1) * 1000 }),
            channels: new Map(),
            soundfonts: new Map(),
        })).then((audio) => {
            audio.reverb.connect(master);
            return audio;
        });
    }
    audioContext.resume();
    return audio;
}

const SOUNDFONT_URL = 'https://gleitz.github.io/midi-js-soundfonts/MusyngKite';
const SEMITONES = { C: 0, D: 2, E: 4, F: 5, G: 7, A: 9, B: 11 };

// The MIDI number of a note name of the soundfont, like "Bb3" or "C4".
function midiOf(name) {
    const [, letter, accidental, octave] = name.match(/^([A-G])([b#]?)(-?\d+)$/);
    return (Number(octave) + 1) * 12 + SEMITONES[letter] + (accidental === '#' ? 1 : accidental === 'b' ? -1 : 0);
}
// The samples of the soundfont are quiet.
const SOUNDFONT_GAIN = 5;

// The samples of an instrument of the soundfont by note name, decoded once for all its instances
// (one per voice): smplr would decode them again for each, 50 MB each.
function loadSoundfontSamples({ context, soundfonts }, name) {
    if (!soundfonts.has(name)) {
        soundfonts.set(name, (async () => {
            const response = await fetch(`${SOUNDFONT_URL}/${name}-ogg.js`);
            if (!response.ok) throw new Error(`${response.status} ${response.statusText} for ${name}`);
            // After a preamble: "MIDI.Soundfont.name = {...,\n}", the data URIs of the samples by note name.
            const source = await response.text();
            const start = source.indexOf('=', source.indexOf('MIDI.Soundfont.')) + 1;
            const dataUris = JSON.parse(`${source.slice(start, source.lastIndexOf(','))}}`);
            const decoded = await Promise.all(Object.entries(dataUris).map(async ([note, dataUri]) => {
                const bytes = Uint8Array.from(atob(dataUri.slice(dataUri.indexOf(',') + 1)), (char) => char.charCodeAt(0));
                return [note, await context.decodeAudioData(bytes.buffer)];
            }));
            // In the order of their pitch: smplr pairs samples and key ranges by index after sorting the ranges.
            const samples = {};
            for (const [note, buffer] of decoded.sort(([a], [b]) => midiOf(a) - midiOf(b))) samples[note] = buffer;
            return samples;
        })().catch((error) => {
            soundfonts.delete(name);
            throw error;
        }));
    }
    return soundfonts.get(name);
}

// A channel of the mix: an instrument for a voice, with a gain for its volume. Made in the context
// of the bus, the live one or one rendered offline; the samples are those of the live one.
async function createChannel(audio, { context, master, reverb, scheduler }, name, voice) {
    const { smplr, loader } = audio;
    let instrument;
    if (name === 'grand_piano') {
        instrument = smplr.SplendidGrandPiano(context, { volume: 90, destination: master, loader, scheduler });
    } else {
        const samples = await loadSoundfontSamples(audio, name);
        // The Sampler takes undefined for a value it is not given, so all are given.
        instrument = smplr.Sampler(context, { buffers: samples, decayTime: 0.3, detune: 0, lpfCutoffHz: 20000, volume: 90, destination: master, loader, scheduler });
        const gain = context.createGain();
        gain.gain.value = SOUNDFONT_GAIN;
        instrument.output.addInsert(gain);
    }
    const gain = context.createGain();
    instrument.output.addInsert(gain);
    instrument.output.addEffect('reverb', reverb, 0);
    await instrument.ready;
    return { voice, instrument, gain };
}

async function loadChannel(name, voice) {
    const audio = await useAudio();
    const key = `${name} ${voice}`;
    if (!audio.channels.has(key)) {
        audio.channels.set(key, createChannel(audio, audio, name, voice).catch((error) => {
            // Loaded again on the next play, e.g. when the network is back.
            audio.channels.delete(key);
            throw error;
        }));
    }
    return audio.channels.get(key);
}

function mixChannel({ voice, instrument, gain }, { reverb, voices }) {
    const { isMuted, volume, pan, reverb: voiceReverb } = voices[voice];
    gain.gain.value = isMuted ? 0 : gainOf(volume);
    instrument.output.pan = pan / 100;
    instrument.output.setEffectMix('reverb', reverb / 100 * voiceReverb / 100);
}

function mixChannels(settings) {
    audio?.then(({ channels }) => {
        for (const loading of channels.values()) loading.then((channel) => mixChannel(channel, settings)).catch(() => {});
    });
}

function startNote(instrument, note, layers, { time, duration, stopId }) {
    for (const { transposition, level } of layers) {
        instrument.start({ note: note.pitch + transposition, velocity: Math.round(note.velocity * level), time, duration, stopId });
    }
}

const layersPerTrackOf = (performance, { instrument, voices }) => voicesByTrack(performance.voiceCount).map((voice) => layersPerNote(instrument, voices[voice].organStops));

// Long enough for the reverb to fade out after the last note.
const REVERB_TAIL_SECONDS = 4;

// The performance as the player sounds it, rendered offline as fast as it can be.
async function renderPerformance(performance, settings) {
    const audio = await useAudio();
    const rate = settings.tempo / performance.tempo;
    const duration = performance.duration / 1000 / rate + REVERB_TAIL_SECONDS;
    return audio.smplr.renderOffline(async (context) => {
        const master = context.createGain();
        master.gain.value = gainOf(settings.volume);
        master.connect(context.destination);
        const reverb = audio.smplr.Reverb(context);
        reverb.connect(master);
        await reverb.ready();
        // Hands all notes to the audio clock at once: offline, a timer of smplr would miss most of them.
        const scheduler = audio.smplr.Scheduler(context, { lookaheadMs: duration * 1000 });
        const bus = { context, master, reverb, scheduler };
        const channels = await Promise.all(voicesByTrack(performance.voiceCount).map((voice) => createChannel(audio, bus, settings.instrument, voice)));
        for (const channel of channels) mixChannel(channel, settings);
        const layersPerTrack = layersPerTrackOf(performance, settings);
        for (const note of performance.notes) {
            startNote(channels[note.voice].instrument, note, layersPerTrack[note.voice], {
                time: note.start / 1000 / rate,
                duration: (note.end - note.start) / 1000 / rate,
            });
        }
    }, { duration, sampleRate: audio.context.sampleRate });
}

function lineStartsOf(performance) {
    const starts = new Map();
    for (const { id, start } of performance.soundings) {
        const line = lineAndVoiceOf(id)?.line;
        if (line && !(starts.get(line) <= start)) starts.set(line, start);
    }
    return starts;
}

// Only one player sounds at a time.
let soundingPlayer = null;
// The player last played or clicked into, which the space bar toggles.
let currentPlayer = null;

export function toggleCurrentScorePlayer(fallback) {
    const player = currentPlayer?.hasScore ? currentPlayer : (soundingPlayer ?? fallback);
    player?.toggle();
}

// Plays a score as Verovio shows it, transposed or with voices left out: PlayableHighlightedScore
// connects it to the toolkit the score is loaded into. position and duration are in milliseconds
// of the score, toSeconds() turns them into seconds of the tempo it is played in. soundingIds are
// the ids of the elements that sound at the position, also while it pauses. A reactive object, so
// the components follow its state, which the functions below set in place.
export function useScorePlayer() {
    const settings = useScorePlayerSettings();

    const player = reactive({
        hasScore: false,
        isLoading: false,
        isDownloading: false,
        isPlaying: false,
        position: 0,
        duration: 0,
        soundingIds: [],
        // The first attack of each line of the drawn score, in milliseconds.
        lineStarts: new Map(),
        connect,
        disconnect,
        play,
        pause,
        toggle,
        stop,
        seek,
        goTo,
        goToLine,
        step,
        download,
        toSeconds,
        makeCurrent,
    });

    let callVerovioMethod = null;
    let performancePromise = null;
    let scoreTempo = 120;
    // The sounding playback: its sound stops with end(), its tempo changes with setTempo().
    let session = null;

    // Connects the score as it is loaded into Verovio now; loaded again, e.g. transposed, it goes on where it was.
    async function connect(method) {
        const wasPlaying = player.isPlaying;
        pause();
        callVerovioMethod = method;
        performancePromise = null;
        player.hasScore = true;
        const performance = await currentPerformance();
        if (!performance || callVerovioMethod !== method) return;
        player.position = Math.min(player.position, performance.duration);
        if (wasPlaying) play();
        else if (player.soundingIds.length) seek(player.position);
    }

    function disconnect() {
        stop();
        if (currentPlayer === player) currentPlayer = null;
        callVerovioMethod = null;
        performancePromise = null;
        player.hasScore = false;
        player.duration = 0;
        player.lineStarts = new Map();
    }

    function loadPerformance() {
        performancePromise ??= loadScorePerformance(callVerovioMethod).then((performance) => {
            player.duration = performance.duration;
            scoreTempo = performance.tempo;
            player.lineStarts = lineStartsOf(performance);
            return performance;
        }, (error) => {
            performancePromise = null;
            throw error;
        });
        return performancePromise;
    }

    // The performance of the score connected now, or null when it cannot be loaded or another
    // score was connected meanwhile.
    async function currentPerformance() {
        const promise = loadPerformance();
        try {
            const performance = await promise;
            return promise === performancePromise ? performance : null;
        } catch (error) {
            console.error(error);
            return null;
        }
    }

    async function play() {
        if (!callVerovioMethod) return;
        pause();
        if (soundingPlayer && soundingPlayer !== player) soundingPlayer.pause();
        soundingPlayer = currentPlayer = player;

        const thisSession = {};
        session = thisSession;
        player.isLoading = player.isPlaying = true;
        try {
            const { context, master } = await useAudio();
            master.gain.value = gainOf(settings.volume);
            const performance = await loadPerformance();
            const channels = await Promise.all(voicesByTrack(performance.voiceCount).map((voice) => loadChannel(settings.instrument, voice)));
            mixChannels(settings);
            if (session !== thisSession) return;
            player.isLoading = false;
            startSession(thisSession, performance, channels.map(({ instrument }) => instrument), context);
        } catch (error) {
            // The position is kept for the next attempt.
            if (session === thisSession) pause();
            console.error(error);
        }
    }

    // Sounds the notes from the position on, a note already sounding there for the rest of it.
    function startSession(thisSession, performance, instruments, context) {
        const from = Math.min(Math.max(player.position, 0), performance.duration);

        // Which time of the score sounds at which time of the audio context.
        let clock = { scoreTime: from, audioTime: context.currentTime + 0.05, rate: settings.tempo / performance.tempo };
        const scoreTimeAt = (audioTime) => clock.scoreTime + (audioTime - clock.audioTime) * 1000 * clock.rate;
        const audioTimeAt = (scoreTime) => clock.audioTime + (scoreTime - clock.scoreTime) / 1000 / clock.rate;

        const layersPerTrack = layersPerTrackOf(performance, settings);
        // The notes handed to the audio clock, with their index, which is also their stopId.
        const scheduled = [];
        let next = 0;
        function scheduleNotes() {
            const until = scoreTimeAt(context.currentTime + LOOKAHEAD_SECONDS);
            for (; next < performance.notes.length && performance.notes[next].start < until; next++) {
                const note = performance.notes[next];
                const { end } = note;
                if (end <= from) continue;
                const start = Math.max(note.start, from);
                const audioTime = audioTimeAt(start);
                scheduled.push({ index: next, audioTime });
                startNote(instruments[note.voice], note, layersPerTrack[note.voice], { time: audioTime, duration: (end - start) / 1000 / clock.rate, stopId: next });
            }
        }

        // Sets the clock anew from where playback is, so the sounding notes go on, and hands the
        // notes not yet sounding over anew.
        thisSession.setTempo = (tempo) => {
            const audioTime = Math.max(context.currentTime, clock.audioTime);
            clock = { scoreTime: scoreTimeAt(audioTime), audioTime, rate: tempo / performance.tempo };
            const first = scheduled.findIndex((note) => note.audioTime > audioTime);
            if (first === -1) return;
            for (const { index } of scheduled.splice(first)) {
                for (const instrument of instruments) instrument.stop({ stopId: index });
            }
            next = scheduled.at(-1) ? scheduled.at(-1).index + 1 : 0;
            scheduleNotes();
        };

        let frame = null;
        function followPosition() {
            const now = scoreTimeAt(context.currentTime);
            if (now >= performance.duration) {
                stop();
                return;
            }
            player.position = Math.max(from, now);
            const ids = idsSoundingAt(performance, now);
            if (ids.join() !== player.soundingIds.join()) player.soundingIds = ids;
            frame = requestAnimationFrame(followPosition);
        }

        scheduleNotes();
        const interval = setInterval(scheduleNotes, SCHEDULE_INTERVAL_MS);
        frame = requestAnimationFrame(followPosition);
        thisSession.end = () => {
            clearInterval(interval);
            cancelAnimationFrame(frame);
            for (const instrument of instruments) instrument.stop();
        };
    }

    // Keeps the position and the elements that sound there.
    function pause() {
        session?.end?.();
        session = null;
        if (soundingPlayer === player) soundingPlayer = null;
        player.isLoading = player.isPlaying = false;
    }

    function toggle() {
        if (player.isPlaying) pause();
        else play();
    }

    function stop() {
        pause();
        player.position = 0;
        player.soundingIds = [];
    }

    // Goes to a time of the score, in milliseconds, and plays on from there if it plays.
    async function seek(time) {
        player.position = time;
        if (player.isPlaying) {
            play();
            return;
        }
        if (!callVerovioMethod) return;
        const performance = await currentPerformance();
        if (performance) player.soundingIds = idsSoundingAt(performance, time);
    }

    // Goes to the first time the element with the given id sounds, e.g. a note.
    async function goTo(id) {
        if (!callVerovioMethod) return;
        const sounding = (await currentPerformance())?.soundings.find((sounding) => sounding.id === id);
        if (sounding) seek(sounding.start);
    }

    // The score as it sounds, as 'midi' or 'wav', in a file of the given name and the format's extension.
    async function download(format, fileName) {
        if (!callVerovioMethod || player.isDownloading) return;
        player.isDownloading = true;
        try {
            const performance = await currentPerformance();
            if (!performance) return;
            const blob = format === 'wav'
                ? (await renderPerformance(performance, settings)).toWav16()
                : new Blob([await performanceToMidi(performance, settings)], { type: 'audio/midi' });
            const url = URL.createObjectURL(blob);
            const link = document.createElement('a');
            link.href = url;
            link.download = `${fileName}.${format === 'wav' ? 'wav' : 'mid'}`;
            link.click();
            URL.revokeObjectURL(url);
        } catch (error) {
            console.error(error);
        } finally {
            player.isDownloading = false;
        }
    }

    function makeCurrent() {
        currentPlayer = player;
    }

    // Goes to the first attack of a line of the drawn score.
    function goToLine(line) {
        const start = player.lineStarts.get(line);
        if (start !== undefined) seek(start);
    }

    // Goes to the next (1) or previous (-1) sonority and returns the line attacked there, or null at
    // either end of the score.
    async function step(direction) {
        if (!callVerovioMethod) return null;
        const performance = await currentPerformance();
        if (!performance) return null;
        const starts = [...new Set(performance.soundings.map(({ start }) => start))].sort((a, b) => (a - b) * direction);
        const sounding = Math.max(...starts.filter((start) => start <= player.position + SAME_ATTACK_MS));
        const target = starts.find((start) => (start - sounding) * direction > SAME_ATTACK_MS);
        if (target === undefined) return null;
        seek(target);
        return Math.max(...performance.soundings.filter(({ start }) => start === target).map(({ id }) => lineAndVoiceOf(id)?.line ?? 0));
    }

    function toSeconds(time) {
        return time / 1000 * scoreTempo / settings.tempo;
    }

    // Another tempo or mix takes effect at once, another sound starts the sounding notes again.
    watch(() => settings.tempo, (tempo) => session?.setTempo?.(tempo));
    watch(() => settings.volume, (volume) => audio?.then(({ master }) => (master.gain.value = gainOf(volume))));
    watch(() => [settings.reverb, settings.voices], () => mixChannels(settings), { deep: true });
    watch(() => [settings.instrument, ...VOICES.map((voice) => settings.voices[voice].organStops.join())], () => {
        if (player.isPlaying && !player.isLoading) play();
    });

    onScopeDispose(disconnect);

    return player;
}

if (import.meta.hot) {
    import.meta.hot.accept(acceptHMRUpdate(useScorePlayerSettings, import.meta.hot));
}
