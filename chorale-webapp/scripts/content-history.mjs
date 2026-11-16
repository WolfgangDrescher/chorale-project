// Writes the last commit of every content file to content-history.json, for builds without git
// (Docker). Used by modules/page-meta.ts.
import { execFileSync } from 'node:child_process';
import { existsSync, readFileSync } from 'node:fs';
import { dirname, join, relative } from 'node:path';
import { fileURLToPath } from 'node:url';
import { writeIfChanged } from './write-if-changed.mjs';

const webapp = join(dirname(fileURLToPath(import.meta.url)), '..');
const content = join(webapp, 'content');
export const HISTORY_FILE = join(webapp, 'content-history.json');

// '' without git or without a repository.
function git(...args) {
    try {
        return execFileSync('git', args, { cwd: webapp, stdio: ['ignore', 'pipe', 'ignore'] }).toString().trim();
    } catch {
        return '';
    }
}

// "faq/tenor-range.md" => { commit, date, author }, or null without a repository.
// In a shallow clone the boundary commits seem to change every older file: those files are
// left out, or the clone is refused with `complete`.
export function readHistoryFromGit({ complete = false } = {}) {
    const repo = git('rev-parse', '--show-toplevel');
    if (!repo) return null;
    const shallow = git('rev-parse', '--is-shallow-repository') === 'true';
    if (shallow && complete) return null;
    const boundary = shallow ? readFileSync(git('rev-parse', '--path-format=absolute', '--git-path', 'shallow'), 'utf8').split('\n') : [];
    const history = {};
    const seen = new Set();
    let current;
    for (const line of git('log', '--format=@%H %as %an', '--name-only', '--', content).split('\n')) {
        if (line.startsWith('@')) {
            const [commit, date, ...author] = line.slice(1).split(' ');
            current = { commit, date, author: author.join(' ') };
        } else if (line && current && !seen.has(line)) {
            seen.add(line);
            if (!boundary.includes(current.commit)) history[relative(content, join(repo, line))] = current;
        }
    }
    return history;
}

export function readHistoryFromFile() {
    return existsSync(HISTORY_FILE) ? JSON.parse(readFileSync(HISTORY_FILE, 'utf8')) : null;
}

if (process.argv[1] === fileURLToPath(import.meta.url)) {
    const history = readHistoryFromGit({ complete: true });
    if (!history) {
        console.error('❌ The history of the content cannot be read: this needs a git repository with its full history (not a shallow clone).');
        process.exit(1);
    }
    const sorted = Object.fromEntries(Object.entries(history).sort(([a], [b]) => a.localeCompare(b)));
    const changed = writeIfChanged(HISTORY_FILE, `${JSON.stringify(sorted, null, 4)}\n`);
    console.log(`✅ Last commit of ${Object.keys(sorted).length} content files${changed ? ' written to content-history.json' : ' (unchanged)'}`);
}
