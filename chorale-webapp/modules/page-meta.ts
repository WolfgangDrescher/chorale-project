import { addTemplate, defineNuxtModule } from '@nuxt/kit';
import { existsSync, readdirSync, statSync } from 'node:fs';
import { join } from 'node:path';
// @ts-expect-error a plain script, shared with `make history`
import { readHistoryFromFile, readHistoryFromGit } from '../scripts/content-history.mjs';

// Provides the last commit of each page as #build/content-history.mjs, keyed by its stem
// ("faq/tenor-range"). Not a content hook: Nuxt Content caches parsed pages and a commit changes no text.
// Without git (Docker) the commits come from content-history.json, see `make history`.

const COLLECTION_DIRECTORIES = ['lessons', 'topics', 'faq'];

export default defineNuxtModule({
    meta: { name: 'page-meta' },
    setup(_, nuxt) {
        const content = join(nuxt.options.rootDir, 'content');
        addTemplate({
            filename: 'content-history.mjs',
            getContents() {
                const git = readHistoryFromGit();
                const history = git ?? readHistoryFromFile() ?? {};
                const pages: Record<string, { commit: string, date: string }> = {};
                for (const directory of COLLECTION_DIRECTORIES.filter((directory) => existsSync(join(content, directory)))) {
                    for (const name of readdirSync(join(content, directory)).filter((file) => file.endsWith('.md'))) {
                        const file = `${directory}/${name}`;
                        // An uncommitted page shows when its file was written.
                        const last = history[file] ?? (git ? { commit: '', date: statSync(join(content, file)).mtime.toISOString().slice(0, 10) } : null);
                        if (last) pages[file.slice(0, -'.md'.length)] = { commit: last.commit, date: last.date };
                    }
                }
                return `export default ${JSON.stringify(pages, null, 4)};\n`;
            },
        });
    },
});
