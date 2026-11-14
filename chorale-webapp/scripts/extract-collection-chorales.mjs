import fs from 'node:fs';
import { basename, dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { dump } from 'js-yaml';
import { CHORALE_COLLECTIONS } from '../shared/utils/chorale-collections.js';
import { parseHumdrumReferenceRecords } from './humdrum-reference-records.mjs';
import { writeIfChanged } from './write-if-changed.mjs';

// One YAML file per score of the collections in chorale-collections.js, read from their submodules.

const __dirname = dirname(fileURLToPath(import.meta.url));
const projectRoot = `${__dirname}/../..`;
const yamlRoot = `${__dirname}/../content/collection-chorales`;

const onlyChanged = process.argv.includes('--only-changed');

// The first key and key signature of the melody, the last spine: *e: and *k[f#] give e and 1.
function keyOf(kern) {
    const tokens = kern.split(/\r?\n/).filter((line) => line.startsWith('*')).map((line) => line.split('\t').at(-1));
    const key = tokens.map((token) => token.match(/^\*([a-gA-G][#-]*):$/)?.[1]).find(Boolean);
    const signature = tokens.map((token) => token.match(/^\*k\[(.*)\]$/)?.[1]).find((value) => value !== undefined);
    if (!key) return {};
    return {
        key,
        mode: key === key.toUpperCase() ? 'major' : 'minor',
        ...(signature !== undefined && { signature: (signature.match(/#/g) ?? []).length - (signature.match(/-/g) ?? []).length }),
    };
}

// Every meter of the score, in the order they first appear.
const metersOf = (kern) => [...new Set([...kern.matchAll(/^\*M(\d+\/\d+)/gm)].map((match) => match[1]))];

function choraleConfig(collection, id, kern) {
    const records = parseHumdrumReferenceRecords(kern);
    const config = {
        collection: collection.id,
        choraleId: id,
        // Not always a number: Schiørring has 30a and 30b.
        number: records.ONM ?? null,
        title: records[collection.titleRecord] ?? null,
        composer: records.COM ?? null,
        meters: metersOf(kern),
        ...keyOf(kern),
    };
    for (const name of Object.keys(config)) {
        if (config[name] === null || (Array.isArray(config[name]) && !config[name].length)) delete config[name];
    }
    return config;
}

if (!onlyChanged) fs.rmSync(yamlRoot, { recursive: true, force: true });

for (const collection of CHORALE_COLLECTIONS) {
    const kernDir = `${projectRoot}/${collection.directory}/kern`;
    const yamlDir = `${yamlRoot}/${collection.id}`;
    if (!fs.existsSync(kernDir)) {
        console.error(`❌ ${kernDir} does not exist. Run \`git submodule update --init\` in the project root first.`);
        process.exit(1);
    }
    fs.mkdirSync(yamlDir, { recursive: true });

    const files = fs.readdirSync(kernDir).filter((file) => file.endsWith('.krn')).sort();
    const yamlFiles = new Set();
    let changedCount = 0;
    for (const file of files) {
        const id = basename(file, '.krn');
        const config = choraleConfig(collection, id, fs.readFileSync(`${kernDir}/${file}`, 'utf8'));
        const yaml = dump(config, { indent: 4, lineWidth: -1, sortKeys: true });
        yamlFiles.add(`${id}.yaml`);
        if (writeIfChanged(`${yamlDir}/${id}.yaml`, yaml)) changedCount++;
    }
    for (const file of fs.readdirSync(yamlDir)) {
        if (file.endsWith('.yaml') && !yamlFiles.has(file)) {
            fs.rmSync(`${yamlDir}/${file}`);
            changedCount++;
        }
    }
    console.log(`✅ Extract metadata for ${files.length} chorales of ${collection.id} (${changedCount} files changed)`);
}
