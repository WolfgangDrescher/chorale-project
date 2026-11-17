// A player per chorale, kept while the chorale stays in the list and stopped when it leaves.
export function useScorePlayers(getChoraleIds) {
    const entries = new Map();
    const players = shallowRef(new Map());
    watch(getChoraleIds, (choraleIds) => {
        for (const [choraleId, { scope }] of entries) {
            if (!choraleIds.includes(choraleId)) {
                scope.stop();
                entries.delete(choraleId);
            }
        }
        for (const choraleId of choraleIds) {
            if (entries.has(choraleId)) continue;
            const scope = effectScope();
            entries.set(choraleId, { scope, player: scope.run(useScorePlayer) });
        }
        players.value = new Map(choraleIds.map((choraleId) => [choraleId, entries.get(choraleId).player]));
    }, { immediate: true });
    onScopeDispose(() => {
        for (const { scope } of entries.values()) scope.stop();
    });
    return players;
}
