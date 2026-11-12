const HTML_ENTITIES = {
    auml: 'ä',
    ouml: 'ö',
    uuml: 'ü',
    Auml: 'Ä',
    Ouml: 'Ö',
    Uuml: 'Ü',
    szlig: 'ß',
    amp: '&',
};

// The reference records contain HTML entities ("Wasserfl&uuml;ssen").
function decodeHtmlEntities(value) {
    return value.replace(/&(#x[0-9a-f]+|#\d+|[a-z]+);/gi, (entity, name) => {
        if (name[0] === '#') {
            const codePoint = name[1].toLowerCase() === 'x' ? parseInt(name.slice(2), 16) : parseInt(name.slice(1), 10);
            return String.fromCodePoint(codePoint);
        }
        return HTML_ENTITIES[name] ?? entity;
    });
}

export function parseHumdrumReferenceRecords(humdrum) {
    const output = {};
    for (const line of humdrum.split(/\r?\n/)) {
        const matches = line.match(/^!!!\s*([^:]+?)\s*:\s*(.*?)\s*$/);
        if (!matches) continue;
        const key = matches[1];
        const value = decodeHtmlEntities(matches[2]);
        if (Array.isArray(output[key])) {
            output[key].push(value);
        } else if (typeof output[key] !== 'undefined') {
            output[key] = [output[key], value];
        } else {
            output[key] = value;
        }
    }
    return output;
}
