import { defineStore } from 'pinia';

const verovioWorker = new Worker(new URL('../workers/verovio.js', import.meta.url), { type: 'module' });

export const useVerovioWorker = defineStore('verovio_worker', {
    state: () => {
        return {
            verovioWorker,
        };
    },
});
