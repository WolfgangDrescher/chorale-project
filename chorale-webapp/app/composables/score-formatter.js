export function useScoreFormatter() {

    const { localScoreUrlGenerator } = useScoreUrlGenerator();
    const scoreOptions = useScoreOptions();
    const rawScoreData = ref(null);
    const formattedScoreData = ref(null);
    const currentId = ref(null);
    const currentFilters = ref(null);

    watch(() => scoreOptions.showDcmlAnnotations, async (newValue) => {
        rawScoreData.value = null;
        await loadScore(currentId.value, currentFilters.value)
    });

    async function loadScore(id, filters) {
        currentId.value = id;
        currentFilters.value = filters;
        if (!rawScoreData.value) {
            const data = await $fetch(localScoreUrlGenerator(id, scoreOptions.showDcmlAnnotations), {
                parseResponse: (txt) => txt,
            });
            rawScoreData.value = data;
        }
        applyScoreFormatting([], filters);
        return rawScoreData.value;
    }

    function applyScoreFormatting(highlightLineNumbers = [], filters = []) {
        if (!rawScoreData.value) return null;

        const lines = rawScoreData.value.split('\n');

        for (let i = 0; i < lines.length; i++) {
            if (highlightLineNumbers.includes(i + 1)) {
                const tokens = lines[i].split('\t');
                tokens.forEach((_, tokenIndex) => {
                    const resolvedLineIndex = getResolvedTokenLineIndex(i, tokenIndex, lines);
                    if (resolvedLineIndex) {
                        lines[resolvedLineIndex] = lines[resolvedLineIndex]
                            .split('\t')
                            .map((token, ti) => {
                                if (ti === tokenIndex && /^[\[\(]?\d+/.test(token)) {
                                    return `${token}@`;
                                }
                                return token;
                            })
                            .join('\t');
                    }
                });
            }
        }

        if (highlightLineNumbers.length) {
            lines.push('!!!RDF**kern: @ = marked note color="#ef4444"');
        }
        if (filters.length) {
            filters.forEach(filter => lines.push(`!!!filter: ${filter}`));
        }

        formattedScoreData.value = lines.join('\n');
        return formattedScoreData.value;
    }

    return {
        rawScoreData,
        formattedScoreData,
        loadScore,
        applyScoreFormatting,
    };
}

function tokenIsDataRecord(line, includeNullToken = false) {
    return !line.startsWith('!') && !line.startsWith('*') && !line.startsWith('=') && !(!includeNullToken && line === '.');
}

function getResolvedTokenLineIndex(lineIndex, spineIndex, lines) {
    for (let i = lineIndex; i >= 0; i--) {
        const token = lines[i].split('\t')[spineIndex];
        if (token && tokenIsDataRecord(token)) {
            return i;
        }
    }
    return null;
}
