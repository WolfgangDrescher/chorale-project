// Drafts the next entry of the changelog (content/changelog/) from the git history.
//
//   node scripts/changelog-draft.mjs                    writes content/changelog/<date>.md for the next update
//   node scripts/changelog-draft.mjs --prompt           prints a prompt for an AI to write the entry instead
//   node scripts/changelog-draft.mjs --date 2026-11-11  drafts the update of another day
//   node scripts/changelog-draft.mjs --range a1b2c3d..HEAD
//
// An entry is one update of the website, named after its day. It takes the commits that landed
// after the last entry up to the end of that day, by committer date, which a rebase keeps in the
// order of the history unlike the author date.
//
// Its changes are what a visitor notices, so several commits that make up one feature become one
// change that lists all of them. The draft has one change per commit to start from: merge them,
// phrase them for the visitors and delete what nobody notices. Commits already listed in an entry
// are left out, so are the ones that only touch the build, the server or the dependencies, which
// the draft lists in a comment.
import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { parseArgs } from 'node:util';
import { load } from 'js-yaml';

const __dirname = dirname(fileURLToPath(import.meta.url));
const changelogPath = resolve(__dirname, '../content/changelog');

function isoDate(date) {
    return date.toISOString().slice(0, 10);
}

function nextMonday(date) {
    const monday = new Date(Date.UTC(date.getFullYear(), date.getMonth(), date.getDate()));
    monday.setUTCDate(monday.getUTCDate() + (8 - monday.getUTCDay()) % 7);
    return isoDate(monday);
}

const { values: options } = parseArgs({
    options: {
        range: { type: 'string' },
        date: { type: 'string', default: nextMonday(new Date()) },
        prompt: { type: 'boolean', default: false },
        force: { type: 'boolean', default: false },
        help: { type: 'boolean', short: 'h', default: false },
    },
});

if (options.help) {
    console.log(fs.readFileSync(fileURLToPath(import.meta.url), 'utf8').split('\nimport ')[0].replace(/^\/\/ ?/gm, ''));
    process.exit(0);
}

// Subjects of commits nobody visiting the website notices.
const NOISE = [/^npm up\b/i, /^Update bach-370-chorales$/, /^Merge /];
// Files of the build, the server and the dependencies. A commit touching nothing else is internal.
const INTERNAL_FILES = [/^docker\//, /^Dockerfile$/, /^compose\.ya?ml$/, /^Makefile$/, /^\.github\//, /^\.[^/]+$/, /(^|\/)package(-lock)?\.json$/, /^bach-370-chorales$/, /(^|\/)tests?\//];

// A first guess at the kind of change from the verb the subject starts with.
function guessType(subject) {
    if (/^(Fix|Repair|Correct|Avoid|Prevent|Reject|Handle|Restore)\b/i.test(subject)) {
        return 'fix';
    }
    if (/^(Add|Introduce|Offer|Allow|Support|Search|Transpose|Show|Track|Link)\b/i.test(subject)) {
        return 'feature';
    }
    return 'improvement';
}

function git(...args) {
    return execFileSync('git', args, { cwd: __dirname, encoding: 'utf8', maxBuffer: 64 * 1024 * 1024 });
}

function readEntries() {
    if (!fs.existsSync(changelogPath)) {
        return [];
    }
    return fs.readdirSync(changelogPath)
        // A draft of the same day that is written again is not the last entry.
        .filter((file) => file.endsWith('.md') && file !== `${options.date}.md`)
        .map((file) => {
            const source = fs.readFileSync(`${changelogPath}/${file}`, 'utf8');
            const frontMatter = source.match(/^---\n([\s\S]*?)\n---/)?.[1] ?? '';
            return { file, source, ...load(frontMatter) };
        })
        .sort((a, b) => new Date(a.date) - new Date(b.date));
}

const entries = readEntries();
const lastEntry = entries.at(-1);
const listedCommits = entries.flatMap((entry) => entry.changes ?? []).flatMap((change) => change.commits ?? []).map(String);
const isListed = (hash) => listedCommits.some((commit) => hash.startsWith(commit) || commit.startsWith(hash));

const since = lastEntry ? isoDate(new Date(lastEntry.date)) : null;
const logArgs = ['log', '--no-merges', '--reverse', '--format=%x1e%h%x1f%cd%x1f%s%x1f%b%x1f', '--date=short', '--name-only'];
if (options.range) {
    logArgs.push(options.range);
} else {
    // Up to the end of the day of the update, from the end of the day of the last one.
    logArgs.push(`--until=${options.date} 23:59:59`);
    if (since) {
        logArgs.push(`--since=${since} 23:59:59`);
    }
}

const allCommits = git(...logArgs).split('\x1e').filter(Boolean).map((record) => {
    const [hash, date, subject, body, files] = record.split('\x1f');
    return { hash, date, subject, body: body.trim(), files: files.trim().split('\n').filter(Boolean) };
});
const commits = allCommits.filter((commit) => !isListed(commit.hash));

// Counted like the compare view of GitHub.
const lastCommit = allCommits.at(-1)?.hash;
const commitCount = lastCommit && lastEntry?.lastCommit
    ? Number(git('rev-list', '--count', `${lastEntry.lastCommit}..${lastCommit}`).trim())
    : allCommits.length;

const isInternal = (commit) => NOISE.some((pattern) => pattern.test(commit.subject))
    || (commit.files.length > 0 && commit.files.every((file) => INTERNAL_FILES.some((pattern) => pattern.test(file))));
const relevant = commits.filter((commit) => !isInternal(commit));
const internal = commits.filter(isInternal);

if (commits.length === 0) {
    console.error(`No commits up to ${options.date} since ${since ?? 'the beginning'} that are not in the changelog yet.`);
    process.exit(1);
}

const yamlString = (value) => JSON.stringify(value);

if (options.prompt) {
    const example = lastEntry?.source ?? '';
    console.log(`Write the next entry of the changelog of the website "Chorale Project" (Bach's 370 chorales: scores, pattern search, chorale analysis, lessons, docs).

Rules:
- An entry is one update of the website, dated ${options.date}. Output only the Markdown file.
- Front matter: title (a short heading for the main point of the update), description (one sentence), date, lastCommit, commitCount, changes.
- lastCommit: '${lastCommit}' and commitCount: ${commitCount}, exactly as given.
- changes: what visitors of the website notice, in English, from their point of view and without developer jargon.
  Merge commits that together make up one feature into one change and list all their hashes under commits.
  Leave out commits nobody notices (refactoring, build, dependencies).
- type: feature (new), improvement (an existing feature got better) or fix (a bug was fixed).
- commits: hashes in single quotes, e.g. ['114e9dc', '0a893bd'], or YAML reads 1733249 as a number.
- github (optional): issues or pull requests in single quotes, '#12' in this repository or 'owner/repo#12' in another one, e.g. ['#2', 'rism-digital/verovio#4515'] for a bug fixed in Verovio.
- to (optional): the path of the page that shows the change, e.g. /chorale, /search, /chorale-analysis, /lessons, /docs.
- Below the front matter a short Markdown paragraph may introduce a larger new feature. Otherwise leave it empty.
- No em dashes (U+2014).
${example ? `\nThe last entry as an example:\n\n${example.trim()}\n` : ''}
The commits since the last entry (hash, date, subject, body, changed files):
${relevant.map((commit) => `
${commit.hash} ${commit.date} ${commit.subject}${commit.body ? `\n${commit.body.replace(/^/gm, '    ')}` : ''}
    Files: ${commit.files.join(', ')}`).join('\n')}
`);
    process.exit(0);
}

const target = `${changelogPath}/${options.date}.md`;
if (fs.existsSync(target) && !options.force) {
    console.error(`${target} exists already, add --force to overwrite it.`);
    process.exit(1);
}

const draft = `---
# Drafted from ${relevant.length} commits by scripts/changelog-draft.mjs. Merge the commits that make up
# one feature into one change, phrase it for the visitors and delete what nobody notices.
# type: feature, improvement or fix. to: the page that shows the change. github: issues or pull
# requests, '#12' or 'owner/repo#12'. Keep hashes and references in quotes, or YAML reads 1733249
# as a number and # as a comment.
title: TODO
description: TODO
date: ${options.date}
lastCommit: '${lastCommit}'
commitCount: ${commitCount}
changes:
${relevant.map((commit) => {
    // A squashed pull request ends its subject with its number.
    const [, subject, pullRequest] = commit.subject.match(/^(.*?)(?: \((#\d+)\))?$/);
    return `  - type: ${guessType(subject)}
    text: ${yamlString(subject)}
    commits: ['${commit.hash}']${pullRequest ? `\n    github: ['${pullRequest}']` : ''}`;
}).join('\n')}
${internal.length ? `# Internal, probably not worth mentioning:\n${internal.map((commit) => `#   ${commit.hash} ${commit.subject}`).join('\n')}\n` : ''}---
`;

fs.mkdirSync(changelogPath, { recursive: true });
fs.writeFileSync(target, draft);
console.log(`Wrote ${target} with ${relevant.length} commits (${internal.length} internal ones in a comment).`);
