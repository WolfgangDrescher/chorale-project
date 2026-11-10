// A player per chorale, stopped when the chorale leaves the list.
export function useScorePlayers(getChoraleIds) {
    const scopes = new Map();
    const players = shallowRef(new Map());
    watch(getChoraleIds, (choraleIds) => {
        for (const [choraleId, scope] of scopes) {
            if (!choraleIds.includes(choraleId)) {
                scope.stop();
                scopes.delete(choraleId);
            }
        }
        players.value = new Map(choraleIds.map((choraleId) => {
            if (!scopes.has(choraleId)) scopes.set(choraleId, effectScope());
            return [choraleId, scopes.get(choraleId).run(useScorePlayer)];
        }));
    }, { immediate: true });
    onScopeDispose(() => {
        for (const scope of scopes.values()) scope.stop();
    });
    return players;
}
