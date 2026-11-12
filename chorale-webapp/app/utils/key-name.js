// The name of a key as Humdrum writes it ("E-", "f#") in the language of a locale. The case stays:
// lower case for a mode with a minor third.
export function localizedKeyName(key, locale) {
    return locale.startsWith('de') ? germanKeyName(key) : key.replaceAll('-', '♭').replaceAll('#', '♯');
}

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
