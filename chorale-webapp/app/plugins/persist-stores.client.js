// Keeps the state of the stores defined with `persist: true` in the local storage, for option stores,
// which cannot hold the refs of useLocalStorage. Restored once hydrated, the server rendered without it.
export default defineNuxtPlugin((nuxtApp) => {
    nuxtApp.$pinia.use(({ store, options }) => {
        if (!options.persist) return;
        const key = `chorale-${store.$id.replaceAll('_', '-')}`;
        onNuxtReady(() => {
            try {
                const stored = JSON.parse(localStorage.getItem(key) ?? '{}');
                // Options an earlier version had are left out.
                store.$patch(Object.fromEntries(Object.entries(stored).filter(([name]) => name in store.$state)));
            } catch {}
            store.$subscribe((_, state) => {
                try {
                    localStorage.setItem(key, JSON.stringify(state));
                } catch {}
            });
        });
    });
});
