export function parseHumdrumReferenceRecords(humdrum) {
    let lines = humdrum.split(/\r?\n/);
    let output = {};
    for (let i = 0; i < lines.length; i++) {
        const matches = lines[i].match(/^!!!\s*([^:]+)\s*:\s*(.*)\s*$/);
        if (matches) {
            const existingValue = output[matches[1]];
            if (Array.isArray(existingValue)) {
                output[matches[1]].push(matches[2])
            } else if (!Array.isArray(existingValue) && typeof existingValue !== 'undefined') {
                output[matches[1]] = [existingValue, matches[2]]
            } else {
                output[matches[1]] = matches[2];
            }
        }
    }
    return output;
}

export function getHumdrumReferenceRecod(humdrum, key) {
    const refs = parseHumdrumReferenceRecords(humdrum);
    if (Array.isArray(key)) {
        for (let value of key) {
            if (refs[value]) {
                return refs[value];
            }
        }
    }
    return refs[key] || null;
}

export function lineIsDataRecord(line, includeNullToken = false) {
    return !line.startsWith('!') && !line.startsWith('*') && !line.startsWith('=') && !(!includeNullToken && line === '.');
}

export function lineIsFermataEnd(lines, lineIndex, kernSpineIndices) {
    const spineTokens = lines[lineIndex]?.split('\t');
    const checkTokenForFermata = (kernSpineIndex) => {
        return spineTokens[kernSpineIndex]?.includes(';') ?? false;
    };
    if (kernSpineIndices.every(checkTokenForFermata)) {
        return true;
    } else if (kernSpineIndices.some(checkTokenForFermata)) {
        throw new Error('Fermata has notes on other lines');
    }
    return false;
}
