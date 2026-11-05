import { execFile } from 'node:child_process';
import { readFile, writeFile } from 'node:fs/promises';
import { promisify } from 'node:util';

// The hand-made modulations, JSON with comments, which chorale-generate reads over.
const MODULATIONS_FILE = '../annotations/bach-modulations.json';

// The blocks of the file ("chorNNN": [...]) in order, with where their text starts and ends.
function splitBlocks(text) {
    const blocks = [];
    for (const match of text.matchAll(/"(chor\d+)":\s*\[/g)) {
        let index = match.index + match[0].length;
        let depth = 1;
        while (depth && index < text.length) {
            const char = text[index];
            if (char === '"') index = text.indexOf('"', index + 1);
            else if (text.startsWith('//', index)) index = text.indexOf('\n', index);
            else if (char === '[') depth++;
            else if (char === ']') depth--;
            index++;
        }
        blocks.push({ id: match[1], start: match.index, end: index });
    }
    return blocks;
}

const ENTRY_INDENT = '        ';

// A line of a block: an entry with what follows its comma (a comment), or anything else, which
// keeps the position of an entry commented out for the order.
function parseLine(line) {
    const entry = line.match(/^(\s*)\["([^"]+)",\s*"([^"]+)"\],?(.*)$/);
    if (entry) return { indent: entry[1], position: entry[2], key: entry[3], rest: entry[4] };
    return { text: line, position: line.match(/\/\/\s*\["([^"]+)"/)?.[1] };
}

// "5/2+1/2" as a number that sorts the positions in the order of the score.
function sortKeyOf(position) {
    const [measure, beat] = position.split(/\/(.*)/);
    const [whole, part = '0/1'] = beat.split('+');
    const [numerator, denominator] = part.split('/').map(Number);
    return Number(measure) * 1000 + Number(whole) + numerator / denominator;
}

// The lines of a block with the modulations in place of its entries. Every other line, and every
// entry that stays, keeps its text, comments included; a removed entry leaves its comment.
function updateLines(lines, modulations) {
    const keys = new Map(modulations);
    const items = [];
    for (const item of lines.map(parseLine)) {
        if (item.text !== undefined) {
            items.push(item);
        } else if (keys.has(item.position)) {
            items.push({ ...item, key: keys.get(item.position) });
            keys.delete(item.position);
        } else if (item.rest.trim()) {
            items.push({ text: item.indent + item.rest.trim(), position: item.position });
        }
    }
    for (const [position, key] of keys) {
        const before = items.findLastIndex((item) => item.position && sortKeyOf(item.position) < sortKeyOf(position));
        items.splice(before + 1, 0, { indent: ENTRY_INDENT, position, key, rest: '' });
    }
    const last = items.findLastIndex((item) => item.text === undefined);
    return items.map((item, index) => item.text ?? `${item.indent}["${item.position}", "${item.key}"]${index === last ? '' : ','}${item.rest}`);
}

// Writes the modulations of a chorale to the file, in place of the entries of its block or as a
// new block in the order of the ids. All else stays as it is.
export async function writeChoraleModulations(id, modulations) {
    const text = await readFile(MODULATIONS_FILE, 'utf8');
    const blocks = splitBlocks(text);
    const existing = blocks.find((block) => block.id === id);
    if (existing) {
        const [first, ...lines] = text.slice(existing.start, existing.end).split('\n');
        const closing = lines.pop();
        const block = [first, ...updateLines(lines, modulations), closing].join('\n');
        await writeFile(MODULATIONS_FILE, text.slice(0, existing.start) + block + text.slice(existing.end));
        return;
    }
    const block = [`"${id}": [`, ...updateLines([], modulations), '    ]'].join('\n');
    const next = blocks.find((other) => other.id > id);
    const updated = next
        ? `${text.slice(0, next.start)}${block},\n    ${text.slice(next.start)}`
        : `${text.slice(0, blocks.at(-1).end)},\n    ${block}${text.slice(blocks.at(-1).end)}`;
    await writeFile(MODULATIONS_FILE, updated);
}

// Generates the chorale anew in ./kern and ./corpus and returns the warnings of chorale-generate.
// The webapp content is left to `make webapp`, as the development server rebuilds it slowly.
export async function regenerateChorale(id) {
    const { stderr } = await promisify(execFile)('make', ['-C', '..', 'kern', 'corpus', id]);
    return stderr.split('\n').filter((line) => line.startsWith(`${id}:`));
}

// The positions of the lines and the modulations of the chorale as generated now.
export async function readPositions(id) {
    return JSON.parse(await readFile(`../positions/bach-370-chorales/${id}.json`, 'utf8'));
}
