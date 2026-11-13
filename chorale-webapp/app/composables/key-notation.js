import { useLocalStorage } from '@vueuse/core';

// The notation of KEY_NOTATIONS the modulations in the score and the keys to transpose to are named in.
export const useKeyNotation = () => useLocalStorage('chorale-key-notation', 'humdrum');
