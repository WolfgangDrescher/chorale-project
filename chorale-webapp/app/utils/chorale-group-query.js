const READABLE_FEATURES = ['deg', 'mint', 'metweight', 'duration', 'hint-12', 'hint-13', 'hint-14', 'hint-23', 'hint-24', 'hint-34'];

// Groups by the driving feature where it can be read, else by scale degree. Without "limit",
// so every match is counted. Undefined for a list of queries or a set of voice pairs.
export function groupQueryOf(query) {
    if (!isCompleteQuery(query)) return undefined;
    const { feature, limit, ...rest } = query;
    if (feature.startsWith('hint-') && !READABLE_FEATURES.includes(feature)) return undefined;
    return {
        query: { feature, ...rest },
        groupBy: [{ reads: [{ feature: READABLE_FEATURES.includes(feature) ? feature : 'deg' }] }],
    };
}
