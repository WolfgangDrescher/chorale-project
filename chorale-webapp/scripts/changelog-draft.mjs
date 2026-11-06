// Drafts the next entry of the changelog (content/changelog/) from the git history.
//
//   node scripts/changelog-draft.mjs                    writes content/changelog/<date>.md for the next update
//   node scripts/changelog-draft.mjs --prompt           prints a prompt for an AI to write the entry instead
//   node scripts/changelog-draft.mjs --date 2026-11-11  drafts the update of another day
//   node scripts/changelog-draft.mjs --range a1b2c3d..HEAD
//   node scripts/changelog-draft.mjs --offline          without the issues and pull requests closed on GitHub
//   node scripts/changelog-draft.mjs --repo owner/repo  another repository than origin
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
//
// Issues and pull requests closed since the last entry come from the GitHub API (GITHUB_TOKEN is
// optional for a public repository). An issue replaces the pull request that closes it.
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
        offline: { type: 'boolean', default: false },
        repo: { type: 'string' },
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
    return execFileSync('git', args, { cwd: __dirname, encoding: 'utf8', maxBuffer: 64 * 1024 * 1024, stdio: ['ignore', 'pipe', 'pipe'] });
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
const logArgs = ['log', '--no-merges', '--reverse', '--format=%x1e%h%x1f%H%x1f%cd%x1f%s%x1f%b%x1f', '--date=short', '--name-only'];
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
    const [hash, fullHash, date, subject, body, files] = record.split('\x1f');
    return { hash, fullHash, date, subject, body: body.trim(), files: files.trim().split('\n').filter(Boolean) };
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

function githubRepository() {
    if (options.repo) {
        return options.repo;
    }
    try {
        return git('remote', 'get-url', 'origin').trim().match(/github\.com[:/]([\w.-]+\/[\w.-]+?)(?:\.git)?\/?$/)?.[1] ?? null;
    } catch {
        return null;
    }
}

const repository = githubRepository();

function reference(owner, number) {
    return owner && owner.toLowerCase() !== repository?.toLowerCase() ? `${owner}#${number}` : `#${number}`;
}

// 'Fixes #12', 'closes owner/repo#3' or 'Resolves https://github.com/owner/repo/issues/4'
const CLOSING = /\b(?:close[sd]?|fix(?:e[sd])?|resolve[sd]?):?\s+(?:([\w.-]+\/[\w.-]+)?#(\d+)|https:\/\/github\.com\/([\w.-]+\/[\w.-]+)\/(?:issues|pull)\/(\d+))/gi;
function closingReferences(text) {
    return [...(text ?? '').matchAll(CLOSING)].map((match) => reference(match[1] ?? match[3], match[2] ?? match[4]));
}

async function githubGet(path, isLastPage = () => false) {
    const token = process.env.GITHUB_TOKEN || process.env.GH_TOKEN;
    const results = [];
    let url = `https://api.github.com/repos/${repository}${path}`;
    while (url) {
        const response = await fetch(url, {
            headers: {
                'Accept': 'application/vnd.github+json',
                'X-GitHub-Api-Version': '2022-11-28',
                'User-Agent': 'chorale-project-changelog-draft',
                ...(token ? { Authorization: `Bearer ${token}` } : {}),
            },
            signal: AbortSignal.timeout(15000),
        });
        if (!response.ok) {
            const rateLimited = response.headers.get('x-ratelimit-remaining') === '0';
            throw new Error(`${response.status} ${response.statusText}${rateLimited ? ', the rate limit is used up, set GITHUB_TOKEN' : ''}${response.status === 404 && !token ? ', a private repository needs GITHUB_TOKEN' : ''}`);
        }
        const page = await response.json();
        results.push(...page);
        url = isLastPage(page) ? null : response.headers.get('link')?.match(/<([^>]+)>;\s*rel="next"/)?.[1];
    }
    return results;
}

// Of a rebased pull request only the last commit is known.
function pullRequestCommits(mergeCommit) {
    try {
        const [, ...parents] = git('rev-list', '--parents', '-n', '1', mergeCommit).trim().split(' ');
        return parents.length > 1 ? git('rev-list', '--no-merges', `${parents[0]}..${mergeCommit}`).trim().split('\n').filter(Boolean) : [mergeCommit];
    } catch {
        return [];
    }
}

async function closedOnGithub() {
    if (options.offline) {
        return { issues: [], pullRequests: [] };
    }
    if (!repository) {
        console.warn('No repository on GitHub found to ask for closed issues and pull requests, add --repo owner/repo.');
        return { issues: [], pullRequests: [] };
    }
    const from = since ? new Date(`${since}T23:59:59`) : new Date(0);
    const until = new Date(`${options.date}T23:59:59`);
    const inWindow = (date) => date && new Date(date) > from && new Date(date) <= until;
    try {
        const [issues, pullRequests] = await Promise.all([
            githubGet(`/issues?state=closed&per_page=100&since=${from.toISOString()}`),
            githubGet('/pulls?state=closed&sort=updated&direction=desc&per_page=100', (page) => new Date(page.at(-1)?.updated_at ?? 0) <= from),
        ]);
        return {
            issues: issues
                .filter((issue) => !issue.pull_request && inWindow(issue.closed_at) && !['not_planned', 'duplicate'].includes(issue.state_reason))
                .map((issue) => ({ reference: `#${issue.number}`, title: issue.title, body: issue.body ?? '', closed: issue.closed_at.slice(0, 10) })),
            pullRequests: pullRequests
                .filter((pullRequest) => inWindow(pullRequest.merged_at))
                .map((pullRequest) => ({
                    reference: `#${pullRequest.number}`,
                    title: pullRequest.title,
                    body: pullRequest.body ?? '',
                    closed: pullRequest.merged_at.slice(0, 10),
                    closes: [...new Set(closingReferences(`${pullRequest.title}\n${pullRequest.body}`))],
                    commits: pullRequest.merge_commit_sha ? pullRequestCommits(pullRequest.merge_commit_sha) : [],
                })),
        };
    } catch (error) {
        console.warn(`Could not ask GitHub (${repository}) for closed issues and pull requests: ${error.message}. Add --offline to skip it.`);
        return { issues: [], pullRequests: [] };
    }
}

const { issues, pullRequests } = await closedOnGithub();
const pullRequestOf = new Map(pullRequests.map((pullRequest) => [pullRequest.reference, pullRequest]));
const issuesInsteadOfPullRequest = (reference) => pullRequestOf.get(reference)?.closes.length ? pullRequestOf.get(reference).closes : [reference];

function referencesOf(commit) {
    const references = [
        commit.subject.match(/ \((#\d+)\)$/)?.[1],
        ...closingReferences(`${commit.subject}\n${commit.body}`),
        ...pullRequests.filter((pullRequest) => pullRequest.commits.includes(commit.fullHash)).map((pullRequest) => pullRequest.reference),
    ].filter(Boolean);
    return [...new Set(references.flatMap(issuesInsteadOfPullRequest))];
}

for (const commit of commits) {
    commit.github = referencesOf(commit);
}

const listedReferences = new Set(entries.flatMap((entry) => entry.changes ?? []).flatMap((change) => change.github ?? []));
const referenced = new Set(commits.flatMap((commit) => [
    ...commit.github,
    ...pullRequests.filter((pullRequest) => pullRequest.closes.some((issue) => commit.github.includes(issue))).map((pullRequest) => pullRequest.reference),
]));
const isUnreferenced = (reference) => !referenced.has(reference) && !listedReferences.has(reference);
const unreferencedPullRequests = pullRequests.filter((pullRequest) => isUnreferenced(pullRequest.reference));
const closedByUnreferencedPullRequest = new Set(unreferencedPullRequests.flatMap((pullRequest) => pullRequest.closes));
const unreferenced = [
    ...issues.filter((issue) => isUnreferenced(issue.reference) && !closedByUnreferencedPullRequest.has(issue.reference))
        .map((issue) => ({ ...issue, kind: 'issue', references: [issue.reference] })),
    ...unreferencedPullRequests.map((pullRequest) => ({ ...pullRequest, kind: 'pull request', references: issuesInsteadOfPullRequest(pullRequest.reference) })),
];

const yamlString = (value) => JSON.stringify(value);
const yamlReferences = (references) => `[${references.map((reference) => `'${reference}'`).join(', ')}]`;
const excerpt = (text, length = 400) => {
    const line = text.replace(/<!--[\s\S]*?-->/g, '').replace(/\s+/g, ' ').trim();
    return line.length > length ? `${line.slice(0, length)}...` : line;
};

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
  Take the references given with the commits of a change. If a pull request closes an issue, reference only the issue, not both.
- to (optional): the path of the page that shows the change, e.g. /chorale, /search, /chorale-analysis, /lessons, /docs.
- Below the front matter a short Markdown paragraph may introduce a larger new feature. Otherwise leave it empty.
- No em dashes (U+2014).
${example ? `\nThe last entry as an example:\n\n${example.trim()}\n` : ''}
The commits since the last entry (hash, date, subject, body, changed files):
${relevant.map((commit) => `
${commit.hash} ${commit.date} ${commit.subject}${commit.body ? `\n${commit.body.replace(/^/gm, '    ')}` : ''}
    Files: ${commit.files.join(', ')}${commit.github.length ? `\n    GitHub: ${commit.github.join(', ')}` : ''}`).join('\n')}
${unreferenced.length ? `
Issues and pull requests closed on GitHub since the last entry that no commit refers to. Add the references to the change they belong to, judging by their title and description, or leave them out if no change fits:
${unreferenced.map((item) => `
${item.references.join(', ')} ${item.title}
    ${item.kind === 'pull request' ? `Pull request ${item.reference}, merged` : 'Issue, closed'} ${item.closed}${item.body ? `: ${excerpt(item.body)}` : ''}`).join('\n')}
` : ''}`);
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
    const subject = commit.subject.replace(/ \(#\d+\)$/, '');
    return `  - type: ${guessType(subject)}
    text: ${yamlString(subject)}
    commits: ['${commit.hash}']${commit.github.length ? `\n    github: ${yamlReferences(commit.github)}` : ''}`;
}).join('\n')}
${internal.length ? `# Internal, probably not worth mentioning:\n${internal.map((commit) => `#   ${commit.hash} ${commit.subject}${commit.github.length ? ` ${commit.github.join(', ')}` : ''}`).join('\n')}\n` : ''}${unreferenced.length ? `# Closed on GitHub since the last entry but no commit refers to them. Add them to the github list
# of the change they belong to:
${unreferenced.map((item) => `#   ${yamlReferences(item.references)} ${item.title}${item.kind === 'pull request' ? ` (pull request ${item.reference})` : ''}`).join('\n')}\n` : ''}---
`;

fs.mkdirSync(changelogPath, { recursive: true });
fs.writeFileSync(target, draft);
console.log(`Wrote ${target} with ${relevant.length} commits (${internal.length} internal ones in a comment)${unreferenced.length ? `, and ${unreferenced.length} closed on GitHub in a comment` : ''}.`);
