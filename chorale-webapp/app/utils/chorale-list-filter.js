// Turns the filter of the chorale list into conditions: a meter like `3/4`, which one of the meters
// of a chorale has to be, a key with or
// without its mode like `G major` or `f# min`, a mode on its own like `dorian`, which is also
// looked for in the mode of the cantus firmus, and the words that are left, which are looked
// for in the id, the BWV number and the title.

const MODES = ['major', 'minor', 'ionian', 'dorian', 'phrygian', 'lydian', 'mixolydian', 'aeolian', 'locrian'];

// A mode may be shortened to its first three letters or more, like `maj` or `min`: from three
// letters on, every prefix belongs to a single mode.
function parseMode(token) {
    return token?.length >= 3 ? MODES.find((mode) => mode.startsWith(token)) ?? null : null;
}

// A key as the scores write it: `G`, `f#`, `B-`.
const KEY = /^[a-g][#-]?$/;

export function parseChoraleListFilter(value) {
    const tokens = value.trim().toLowerCase().split(/\s+/).filter(Boolean);
    const filter = { meters: [], key: null, modes: [], words: [] };

    for (let i = 0; i < tokens.length; i++) {
        const token = tokens[i];

        if (/^\d+\/\d+$/.test(token)) {
            filter.meters.push(token);
        } else if (KEY.test(token) && !filter.key) {
            // Together with its mode, the key is a single condition: `G major` is not `g minor`.
            // A chorale has only one key, so a later letter like the `f` of `g minor f` is a word.
            const mode = parseMode(tokens[i + 1]);
            if (mode) i++;
            filter.key = { key: token, mode };
        } else if (parseMode(token)) {
            filter.modes.push(parseMode(token));
        } else {
            filter.words.push(token);
        }
    }

    return filter;
}

export function matchesChoraleListFilter(chorale, filter) {
    const key = chorale.key?.toLowerCase();
    const text = [chorale.choraleId, chorale.bwv, chorale.title].filter(Boolean).join(' ').toLowerCase();

    return filter.meters.every((meter) => chorale.meters?.includes(meter))
        && (!filter.key || (key === filter.key.key && (!filter.key.mode || chorale.mode === filter.key.mode)))
        && filter.modes.every((mode) => chorale.mode === mode || chorale.cantusFirmus?.mode === mode)
        && filter.words.every((word) => text.includes(word));
}
