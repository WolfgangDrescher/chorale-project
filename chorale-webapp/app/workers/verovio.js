import createVerovioModule from 'verovio/wasm-hum';
import { VerovioToolkit, enableLog, LOG_OFF, LOG_WARNING } from 'verovio/esm';
import { createVerovioWorker } from 'vue-verovio-canvas';

// Verovio's warnings (e.g. about the kern of a score) show in the browser
// console during development only.
const logLevel = import.meta.env.DEV ? LOG_WARNING : LOG_OFF;

createVerovioWorker(createVerovioModule, VerovioToolkit, enableLog, logLevel);
