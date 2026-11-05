import { onKeyStroke, useDebounceFn, useTimeoutFn } from '@vueuse/core';

const REPOSITORY_URL = 'https://github.com/WolfgangDrescher/chorale-project';

// The key changes as markers of a HighlightedScore (its lines prop): a line each, labeled with
// the key, "?" for one not named yet.
export function modulationMarkers(modulations) {
    return [{
        color: 'rgb(34 197 94 / 0.4)',
        items: modulations.map(({ line, key }) => ({ lineNumber: line, label: { value: key || '?', position: 'bottom' } })),
    }];
}

// Annotates the modulations of a chorale while active and it is played: an entry is a line and a
// key, or none yet. With saves, as in the development server, they go to bach-modulations.json.
export function useModulationAnnotator(choraleId, player, active, saves = false) {
    const scoreOptions = useScoreOptions();
    // A Humdrum filter can put lines before the data, see lineShiftOf: the entries name the lines
    // of the file, the player those of the drawn score.
    const lineShift = computed(() => lineShiftOf(scoreOptions.humdrumFilters));

    const annotator = reactive({
        active,
        // Sorted by line.
        entries: [],
        // The key being typed and the line of the entry it is for, null while the entry is closed.
        input: null,
        inputLine: null,
        // null, 'saving', 'saved' or 'failed'.
        status: null,
        error: null,
        // What chorale-generate had to say, e.g. a position it found no note at.
        warnings: [],
        // The key changes of the score on the lines of the page, null until the annotation starts.
        modulations: null,
        saves,
        // The line of the file the framed sonority is on: the last one attacked among the sounding ones.
        line: computed(() => {
            const drawnLine = Math.max(0, ...player.soundingIds.map((id) => lineAndVoiceOf(id)?.line ?? 0));
            return drawnLine ? drawnLine - lineShift.value : null;
        }),
        // The block of the chorale as the file writes it, as a file of its own.
        json: computed(() => `{\n    "${choraleId}": [\n${annotations.value.map(([position, key]) => `        ["${position}", "${key}"]`).join(',\n')}\n    ]\n}\n`),
        // Whether the entries differ from what the file holds.
        changed: computed(() => saved.value !== null && JSON.stringify(annotations.value) !== saved.value),
        issueUrl: computed(() => `${REPOSITORY_URL}/issues/new?${new URLSearchParams({ title: `Modulations of ${choraleId}`, body: `\`\`\`json\n${annotator.json}\`\`\`` })}`),
        download,
        markers: computed(() => modulationMarkers(annotator.entries)),
        mark,
        openInput,
        commitInput,
        closeInput,
        resume,
        remove,
        goTo,
        positionOf,
        isAmbiguous,
    });

    // The position of each line, and the first line of each position, which chorale-generate takes.
    let positions = null;
    let lines = null;
    // The first data line, where the key the score starts in is.
    let firstLine = null;

    // The positions and the key changes of the score the page shows, generated with it by `make kern`.
    async function loadScore() {
        const score = await $fetch(`/positions/bach-370-chorales/${choraleId}.json`, { cache: 'no-store' });
        positions = new Map(Object.entries(score.positions).map(([line, position]) => [Number(line), position]));
        lines = new Map();
        for (const [line, position] of positions) {
            if (!lines.has(position)) lines.set(position, line);
        }
        firstLine = positions.keys().next().value ?? null;
        annotator.modulations = score.modulations;
        return annotator.modulations;
    }

    // The key changes of a score generated anew, moved by their positions to the lines of the
    // page, whose score has its key designations elsewhere.
    function showGenerated(modulations) {
        annotator.modulations = modulations
            .map((modulation) => ({ ...modulation, line: lines.get(modulation.position) }))
            .filter(({ line }) => line !== undefined);
    }

    watch(() => annotator.active, async (active) => {
        if (!active) {
            closeInput();
            return;
        }
        if (!positions) {
            annotator.entries = (await loadScore()).map(({ line, key }) => ({ line, key }));
            saved.value = JSON.stringify(annotations.value);
        }
        // Frames the sonority at the position, so the keys have a place to act on.
        if (!player.soundingIds.length) player.seek(player.position);
    }, { immediate: true });

    function positionOf(line) {
        return positions?.get(line) ?? null;
    }

    // Whether chorale-generate would put the entry of the line elsewhere, at an earlier line with
    // the same position, e.g. after a grace note.
    function isAmbiguous(line) {
        const position = positionOf(line);
        return position !== null && lines.get(position) !== line;
    }

    function entryAt(line) {
        return annotator.entries.find((entry) => entry.line === line) ?? null;
    }

    function setEntry(line, key) {
        const entry = entryAt(line);
        if (entry) entry.key = key;
        else annotator.entries = [...annotator.entries, { line, key }].sort((a, b) => a.line - b.line);
    }

    function mark() {
        if (annotator.line && !entryAt(annotator.line)) setEntry(annotator.line, '');
    }

    // Whether the entry was only made for the key being typed, so that it goes again if no key comes.
    let markedForInput = false;

    // Opens the key field at a line, the framed one by default, and marks the line if needed.
    function openInput(key = '', line = annotator.line) {
        if (!line) return;
        if (player.isPlaying) player.pause();
        markedForInput = !entryAt(line);
        if (markedForInput) setEntry(line, '');
        annotator.input = key;
        annotator.inputLine = line;
    }

    function commitInput() {
        // The English names: B for H.
        const key = (annotator.input?.trim() ?? '').replace(/^h/, 'b').replace(/^H/, 'B');
        if (/^[A-Ga-g][#-]*$/.test(key)) setEntry(annotator.inputLine, key);
        closeInput();
    }

    // Plays on from the framed sonority, e.g. once a key is typed.
    function resume() {
        if (!player.isPlaying) player.play();
    }

    function closeInput() {
        if (annotator.input === null) return;
        const { inputLine } = annotator;
        annotator.input = null;
        annotator.inputLine = null;
        if (markedForInput && entryAt(inputLine)?.key === '') remove(inputLine);
        markedForInput = false;
    }

    function remove(line) {
        annotator.entries = annotator.entries.filter((entry) => entry.line !== line);
    }

    // Saves the block as chorNNN.json.
    function download() {
        const url = URL.createObjectURL(new Blob([annotator.json], { type: 'application/json' }));
        const link = document.createElement('a');
        link.href = url;
        link.download = `${choraleId}.json`;
        link.click();
        URL.revokeObjectURL(url);
    }

    function goTo(entry) {
        player.goToLine(entry.line + lineShift.value);
    }

    // Goes to the next (1) or the previous (-1) entry from the framed sonority on, around the ends.
    function goToNeighbour(direction) {
        const { entries, line } = annotator;
        if (!entries.length) return;
        const next = direction > 0 ? entries.find((entry) => entry.line > (line ?? 0)) ?? entries[0] : [...entries].reverse().find((entry) => entry.line < (line ?? Infinity)) ?? entries.at(-1);
        goTo(next);
    }

    // Stepping pauses playback, which plays on shortly after the last step.
    let playsOnAfterStepping = false;
    const playOnSoon = useDebounceFn(() => {
        if (playsOnAfterStepping && !player.isPlaying) player.play();
        playsOnAfterStepping = false;
    }, 300);

    // Goes to the previous or the next sonority. With withEntry, an entry at the framed one moves
    // along, except the key the score starts in, which stays at its start.
    async function step(direction, withEntry) {
        if (player.isPlaying) {
            player.pause();
            playsOnAfterStepping = true;
        }
        if (playsOnAfterStepping) playOnSoon();
        const entry = withEntry && annotator.line !== firstLine ? entryAt(annotator.line) : null;
        const drawnLine = await player.step(direction);
        if (drawnLine === null || !entry) return;
        const line = drawnLine - lineShift.value;
        if (entryAt(line)) {
            annotator.entries = annotator.entries.filter((other) => other !== entry);
        } else {
            entry.line = line;
            annotator.entries = [...annotator.entries].sort((a, b) => a.line - b.line);
        }
    }

    // The annotations as the file holds them: the entries that have a key, with their positions.
    const annotations = computed(() => annotator.entries
        .filter(({ key }) => key)
        .map(({ line, key }) => [positionOf(line), key])
        .filter(([position]) => position));

    // "Saved" shows for a moment only.
    const { start: clearSavedSoon } = useTimeoutFn(() => {
        if (annotator.status === 'saved') annotator.status = null;
    }, 3000, { immediate: false });

    async function save() {
        annotator.status = 'saving';
        annotator.error = null;
        try {
            const { warnings, modulations } = await $fetch(`/api/dev/modulations/${choraleId}`, { method: 'PUT', body: { modulations: annotations.value } });
            annotator.warnings = warnings;
            saved.value = JSON.stringify(annotations.value);
            annotator.status = 'saved';
            clearSavedSoon();
            showGenerated(modulations);
        } catch (error) {
            annotator.status = 'failed';
            annotator.error = error.data?.message ?? error.message;
        }
    }

    // What the file holds, as far as the annotator knows: loaded or saved last.
    const saved = ref(null);

    const saveSoon = useDebounceFn(save, 800);
    watch(() => JSON.stringify(annotations.value), (now) => {
        if (annotator.saves && saved.value !== null && now !== saved.value) saveSoon();
    });

    const takesKeys = () => annotator.active && document.activeElement === document.body;

    onKeyStroke('Enter', (event) => {
        if (!takesKeys()) return;
        event.preventDefault();
        mark();
    });
    onKeyStroke((event) => /^[A-Ha-h]$/.test(event.key), (event) => {
        if (!takesKeys() || event.metaKey || event.ctrlKey || event.altKey) return;
        event.preventDefault();
        openInput(event.key);
    });
    onKeyStroke(['Backspace', 'Delete'], (event) => {
        if (!takesKeys() || !annotator.line) return;
        event.preventDefault();
        remove(annotator.line);
    });
    onKeyStroke('Tab', (event) => {
        if (!takesKeys()) return;
        event.preventDefault();
        goToNeighbour(event.shiftKey ? -1 : 1);
    });
    onKeyStroke(['ArrowLeft', 'ArrowRight'], (event) => {
        if (!takesKeys()) return;
        event.preventDefault();
        step(event.key === 'ArrowRight' ? 1 : -1, event.shiftKey);
    });

    return annotator;
}
