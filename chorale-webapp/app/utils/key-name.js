// The name of a key as Humdrum writes it ("E-", "f#") in the language of a locale. The case stays:
// lower case for a mode with a minor third.
export function localizedKeyName(key, locale) {
    return locale.startsWith('de') ? germanKeyName(key) : withAccidentalSigns(key);
}

// "E-" is "E♭", "f#" is "f♯".
const withAccidentalSigns = (key) => key.replaceAll('-', '♭').replaceAll('#', '♯');

// Upper case also for minor, the mode follows: "f#" is "F♯" (minor).
const englishKeyName = (key) => withAccidentalSigns(key.charAt(0).toUpperCase() + key.slice(1));

// "E-" is "Es", "B-" is "B", "B" is "H", "f#" is "fis".
function germanKeyName(key) {
    const letter = key[0].toUpperCase();
    const flats = (key.match(/-/g) ?? []).length;
    const sharps = (key.match(/#/g) ?? []).length;
    let name = letter === 'B' ? 'H' : letter;
    if (sharps) name += 'is'.repeat(sharps);
    if (flats === 1 && letter === 'B') name = 'B';
    else if (flats && ['A', 'E'].includes(letter)) name += 's' + 'es'.repeat(flats - 1);
    else if (flats) name += 'es'.repeat(flats);
    return isMinorKey(key) ? name.toLowerCase() : name;
}

// The i18n keys of the name of a mode and its short form.
export const MODE_NAMES = {
    major: ['modeMajor', 'modeMajor'],
    minor: ['modeMinor', 'modeMinor'],
    ionian: ['modeIonian', 'modeIonianShort'],
    dorian: ['modeDorian', 'modeDorianShort'],
    phrygian: ['modePhrygian', 'modePhrygianShort'],
    lydian: ['modeLydian', 'modeLydianShort'],
    mixolydian: ['modeMixolydian', 'modeMixolydianShort'],
    aeolian: ['modeAeolian', 'modeAeolianShort'],
    locrian: ['modeLocrian', 'modeLocrianShort'],
};

const ENGLISH_MODE_NAMES = {
    major: 'major',
    minor: 'minor',
    ionian: 'Ionian',
    dorian: 'Dorian',
    phrygian: 'Phrygian',
    lydian: 'Lydian',
    mixolydian: 'Mixolydian',
    aeolian: 'Aeolian',
    locrian: 'Locrian',
};

// The notations a key can be named in, by the i18n keys of their names.
export const KEY_NOTATIONS = {
    humdrum: 'keyNotationHumdrum',
    german: 'keyNotationGerman',
    jazz: 'keyNotationJazz',
    english: 'keyNotationEnglish',
};

// A key as Humdrum writes it ("E-", "b") with its mode in a notation of KEY_NOTATIONS: "E-",
// "Es-Dur", "E♭" or "E♭ major"; "b", "h-Moll", "Bm" or "B minor"; "g:dor", "g-dorisch", "G Dorian".
export function keyNameInNotation(key, mode, notation, t) {
    switch (notation) {
        case 'german':
            return mode ? t('keyWithMode', { key: germanKeyName(key), mode: t(MODE_NAMES[mode][0]) }) : germanKeyName(key);
        case 'jazz':
            if (mode && isChurchMode(mode)) return `${englishKeyName(key)} ${ENGLISH_MODE_NAMES[mode]}`;
            return isMinorKey(key) ? `${englishKeyName(key)}m` : englishKeyName(key);
        case 'english':
            return [englishKeyName(key), ENGLISH_MODE_NAMES[mode]].filter(Boolean).join(' ');
        case 'humdrum':
        default:
            return mode && isChurchMode(mode) ? `${key}:${mode.slice(0, 3)}` : key;
    }
}
