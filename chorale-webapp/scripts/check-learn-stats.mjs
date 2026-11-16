// Checks the `stat` of each question against what its query (`stat.query`) finds in the corpus.
// Exits with 1 if a stat is off; --write corrects the counts instead.
import { execFileSync } from 'node:child_process';
import { existsSync, readdirSync, readFileSync, writeFileSync } from 'node:fs';
import { dirname, join, relative } from 'node:path';
import { fileURLToPath } from 'node:url';
import { load } from 'js-yaml';

const webapp = join(dirname(fileURLToPath(import.meta.url)), '..');
const repo = join(webapp, '..');
const CHORALE_SEARCH = join(repo, 'chorale-search/build/chorale-search');
const CORPUS = join(repo, 'corpus/bach-370-chorales');
const DIRECTORIES = ['faq'];
const write = process.argv.includes('--write');

const counted = new Map();
function count(query) {
    const key = JSON.stringify(JSON.parse(query));
    if (!counted.has(key)) {
        const output = execFileSync(CHORALE_SEARCH, [CORPUS, '--query', key, '--format', 'json', '--group-by-chorale', '--compact-output'], { maxBuffer: 64 * 1024 * 1024, stdio: ['ignore', 'pipe', 'ignore'] }).toString();
        const results = JSON.parse(output.slice(output.indexOf('{')));
        counted.set(key, {
            matches: Object.values(results).reduce((sum, matches) => sum + matches.length, 0),
            chorales: Object.keys(results).length,
        });
    }
    return counted.get(key);
}

let checked = 0;
let written = 0;
const problems = [];

for (const directory of DIRECTORIES.filter((directory) => existsSync(join(webapp, 'content', directory)))) {
    for (const name of readdirSync(join(webapp, 'content', directory)).filter((file) => file.endsWith('.md'))) {
        const path = join(webapp, 'content', directory, name);
        const file = relative(repo, path);
        let text = readFileSync(path, 'utf8');
        const frontMatter = load(text.match(/^---\n([\s\S]*?)\n---\n/)?.[1] ?? '') ?? {};

        const stat = frontMatter.stat;
        if (!stat?.query) continue;
        const query = frontMatter.queries?.find((entry) => entry.id === stat.query);
        if (!query) {
            problems.push(`${file}: the stat names the query "${stat.query}", which the page does not have`);
            continue;
        }
        const found = count(query.query);
        const numbers = [
            ...(stat.confirm ? [] : [['value', String(stat.counts === 'chorales' ? found.chorales : found.matches)]]),
            ...(stat.chorales === undefined ? [] : [['chorales', String(found.chorales)]]),
        ];
        // A value that is no count (the highest note of a voice) is confirmed by the matches of
        // other queries. --write leaves these claims alone.
        for (const claim of stat.confirm ?? []) {
            const confirming = frontMatter.queries?.find((entry) => entry.id === claim.query);
            if (!confirming) {
                problems.push(`${file}: the stat is confirmed by the query "${claim.query}", which the page does not have`);
                continue;
            }
            const matches = String(count(confirming.query).matches);
            if (matches !== claim.matches) problems.push(`${file}: the stat expects ${claim.matches} matches of the query "${claim.query}", it finds ${matches}`);
        }
        checked++;
        for (const [key, expected] of numbers) {
            if (stat[key] === expected) continue;
            if (!write) {
                problems.push(`${file}: the stat says ${key}: ${stat[key]}, the query "${stat.query}" finds ${expected}`);
                continue;
            }
            // Replaces only the number, to keep the formatting of the front matter.
            const block = text.match(/^stat:.*\n(?:[ \t]+.*\n)*/m);
            const number = new RegExp(`(\\b${key}: )"?${stat[key]}"?`);
            if (!block || !number.test(block[0])) {
                problems.push(`${file}: the stat says ${key}: ${stat[key]}, the query "${stat.query}" finds ${expected}, but the line could not be found to write it`);
                continue;
            }
            text = text.replace(block[0], block[0].replace(number, `$1"${expected}"`));
            writeFileSync(path, text);
            written++;
        }
    }
}

for (const problem of problems) console.error(`❌ ${problem}`);
if (problems.length) {
    if (!write) console.error('To correct the stats, run from the project directory: node chorale-webapp/scripts/check-learn-stats.mjs --write');
    process.exit(1);
}
console.log(`✅ ${checked} stats of questions match their queries${written ? ` (${written} corrected)` : ''}`);
