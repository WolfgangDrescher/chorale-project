// The subgroups with their share, plus a row for the omitted matches.
export function analysisGroupsOf(parent) {
    const groups = (parent.groups ?? []).map((group) => ({ ...group, share: group.matchCount / parent.matchCount }));
    if (parent.omittedMatchCount) {
        groups.push({ other: true, matchCount: parent.omittedMatchCount, share: parent.omittedMatchCount / parent.matchCount });
    }
    return groups;
}
