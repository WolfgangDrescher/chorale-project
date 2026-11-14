// The Latin letters a decomposition does not take apart, as ICU's Latin-ASCII spells them.
const SPELLED_OUT_LETTERS = {
    æ: 'ae', ð: 'd', đ: 'd', ħ: 'h', ı: 'i', ĸ: 'q', ł: 'l', ŋ: 'ng', ø: 'o', œ: 'oe', ß: 'ss', þ: 'th', ŧ: 't', '·': '', 'ʼ': "'",
};

// A character as text is searched: lower case, without its accents, a ligature or the long s
// spelled out (ﬅ is st, ſ is s).
function normalizeCharacter(character) {
    const decomposed = character.toLowerCase().normalize('NFKD').replace(/\p{M}/gu, '');
    return Array.from(decomposed, (letter) => SPELLED_OUT_LETTERS[letter] ?? letter).join('');
}

export function normalizeForSearch(text) {
    return Array.from(text, normalizeCharacter).join('');
}

// The normalized text, and for each of its characters the index of the one in `text` it comes from.
export function normalizeForSearchWithOrigins(text) {
    let normalized = '';
    const origins = [];
    let index = 0;
    for (const character of text) {
        const normalizedCharacter = normalizeCharacter(character);
        normalized += normalizedCharacter;
        for (let i = 0; i < normalizedCharacter.length; i++) origins.push(index);
        index += character.length;
    }
    return { normalized, origins };
}
