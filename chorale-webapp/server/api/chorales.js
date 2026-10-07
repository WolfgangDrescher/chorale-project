import { readdir } from 'node:fs/promises';

// The chorales of the corpus (`make kern`), as the ids their files are served under.
const KERN_DIR = '../kern/bach-370-chorales';

// The ids in order, e.g. ["chor001", "chor005", ...]. Empty where the corpus hasn't been built.
export default defineEventHandler(async () => {
    try {
        const files = await readdir(KERN_DIR);
        return files.filter((file) => file.endsWith('.krn')).map((file) => file.slice(0, -'.krn'.length)).sort();
    } catch {
        return [];
    }
});
