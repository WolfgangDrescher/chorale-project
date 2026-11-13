const MINOR_MODES = ['minor', 'dorian', 'phrygian', 'aeolian'];

// A key signature is counted in sharps, flats count negative: 3 is three sharps, -4 four flats.

// The key signature of the major key on each tonic without a sharp or flat. They follow each
// other in fifths, each fifth up one sharp more: F major has one flat, C major none, B major five
// sharps.
const MAJOR_KEY_SIGNATURES = { f: -1, c: 0, g: 1, d: 2, a: 3, e: 4, b: 5 };

// A sharp on the tonic raises the key by seven fifths, so its key signature has seven sharps
// more (F major one flat, F# major six sharps), a flat seven sharps less.
const KEY_SIGNATURE_CHANGE_PER_TONIC_ACCIDENTAL = 7;

// How the key signature of a mode differs from the one of major on the same tonic: A minor has
// no sharps, three less than A major, A dorian one sharp, two less.
const MODE_KEY_SIGNATURE_CHANGES = { lydian: 1, mixolydian: -1, dorian: -2, minor: -3, aeolian: -3, phrygian: -4, locrian: -5 };

// The keys offered have at most this many sharps or flats: one key for every pitch, the pitch of
// six sharps or flats in both spellings.
const MAX_KEY_SIGNATURE = 6;

// Every tonic as **kern spells it, with a flat, without accidental and with a sharp.
const TONICS = [...'cdefgab'].flatMap((letter) => [`${letter}-`, letter, `${letter}#`]);

// The submenu of the score command palette that the key the chorale is transposed to is chosen
// in (see humdrumFilterSubmenus in ScoreOptionsPalette.vue): the keys of the mode of the chorale
// with at most MAX_KEY_SIGNATURE sharps or flats and the key it is written in, which is the default, in
// the order of the circle of fifths from the flats to the sharps.
export function useTransposeKeySubmenu(chorale) {
    const { t } = useI18n();

    const choraleKey = computed(() => toValue(chorale)?.key?.toLowerCase() ?? null);
    const mode = computed(() => toValue(chorale)?.mode ?? null);
    const isMinor = computed(() => MINOR_MODES.includes(mode.value));

    // The key signature of a key in the mode of the chorale, e.g. -4 for A- major: four flats.
    function keySignatureOf(key) {
        const [letter, ...tonicAccidentals] = key;
        const tonicAccidentalChange = tonicAccidentals
            .reduce((sum, accidental) => sum + (accidental === '#' ? 1 : -1), 0) * KEY_SIGNATURE_CHANGE_PER_TONIC_ACCIDENTAL;
        return MAJOR_KEY_SIGNATURES[letter] + tonicAccidentalChange + (MODE_KEY_SIGNATURE_CHANGES[mode.value] ?? 0);
    }

    // The key signature of a key, e.g. "3♭" or "2♯".
    function keySignatureLabel(key) {
        const keySignature = keySignatureOf(key);
        if (!keySignature) return undefined;
        return `${Math.abs(keySignature)}${keySignature < 0 ? '♭' : '♯'}`;
    }

    const keyNotation = useKeyNotation();

    // The tonic is spelled as **kern spells it, upper case for a mode with a major third.
    function keyLabel(key) {
        const tonic = isMinor.value ? key : key.charAt(0).toUpperCase() + key.slice(1);
        return keyNameInNotation(tonic, mode.value, keyNotation.value, t);
    }

    return computed(() => {
        const keys = TONICS
            .filter((key) => Math.abs(keySignatureOf(key)) <= MAX_KEY_SIGNATURE || key === choraleKey.value)
            .sort((a, b) => keySignatureOf(a) - keySignatureOf(b));

        return {
            parameter: 'key',
            placeholder: t('searchKey'),
            valueLabel: keyLabel,
            values: keys.map((key) => {
                const isOriginal = key === choraleKey.value;
                return {
                    value: isOriginal ? null : key,
                    label: isOriginal ? `${keyLabel(key)} (${t('originalKey')})` : keyLabel(key),
                    suffix: keySignatureLabel(key),
                };
            }),
        };
    });
}
