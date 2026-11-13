import { useLocalStorage } from '@vueuse/core';

export const SEARCH_HISTORY_LENGTH = 25;

// Two queries that differ only in whitespace are the same entry.
function normalized(query) {
    try {
        return JSON.stringify(JSON.parse(query));
    } catch {
        return query.trim();
    }
}

// The last successful searches, newest first, stored for the next visit.
export function useSearchHistory() {
    const entries = useLocalStorage('chorale-search-history', [], { initOnMounted: true });

    function remember({ query, matches, choraleCount }) {
        const key = normalized(query);
        const others = entries.value.filter((entry) => normalized(entry.query) !== key);
        const entry = { id: crypto.randomUUID(), query, searchedAt: Date.now(), matches, choraleCount };
        entries.value = [entry, ...others].slice(0, SEARCH_HISTORY_LENGTH);
    }

    function forget(id) {
        entries.value = entries.value.filter((entry) => entry.id !== id);
    }

    // Back at its place by date, after an undo, unless the query has been searched again since.
    function restore(entry) {
        if (entries.value.some(({ query }) => normalized(query) === normalized(entry.query))) return;
        entries.value = [...entries.value, entry].sort((a, b) => b.searchedAt - a.searchedAt).slice(0, SEARCH_HISTORY_LENGTH);
    }

    function clear() {
        entries.value = [];
    }

    return { entries, remember, forget, restore, clear };
}
