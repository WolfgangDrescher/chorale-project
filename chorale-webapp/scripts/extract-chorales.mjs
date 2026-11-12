import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import { basename, dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { dump } from 'js-yaml';

const __dirname = dirname(fileURLToPath(import.meta.url));

// --only-changed leaves unchanged files alone, for `make webapp` while the dev server runs.
const args = process.argv.slice(2);
const onlyChanged = args.includes('--only-changed');
const [kernDirArg] = args.filter((arg) => !arg.startsWith('--'));

// The corpus `make kern` generates in the project root, or another directory given as an
// argument (e.g. the plain bach-370-chorales/kern sources).
const pathToKernScores = resolve(kernDirArg ?? `${__dirname}/../../kern/bach-370-chorales`);
// What the scores state about themselves as a whole (the key, the meter, the length) comes from
// the chorale-search build, so it is read the same way the search reads it.
const choraleInfoBinary = `${__dirname}/../../chorale-search/build/chorale-info`;
const choralesYamlPath = `${__dirname}/../content/chorales/`;

const HTML_ENTITIES = {
    auml: 'ä',
    ouml: 'ö',
    uuml: 'ü',
    Auml: 'Ä',
    Ouml: 'Ö',
    Uuml: 'Ü',
    szlig: 'ß',
    amp: '&',
};

// The reference records contain HTML entities ("Wasserfl&uuml;ssen").
function decodeHtmlEntities(value) {
    return value.replace(/&(#x[0-9a-f]+|#\d+|[a-z]+);/gi, (entity, name) => {
        if (name[0] === '#') {
            const codePoint = name[1].toLowerCase() === 'x' ? parseInt(name.slice(2), 16) : parseInt(name.slice(1), 10);
            return String.fromCodePoint(codePoint);
        }
        return HTML_ENTITIES[name] ?? entity;
    });
}

function parseHumdrumReferenceRecords(humdrum) {
    const output = {};
    for (const line of humdrum.split(/\r?\n/)) {
        const matches = line.match(/^!!!\s*([^:]+?)\s*:\s*(.*?)\s*$/);
        if (!matches) continue;
        const key = matches[1];
        const value = decodeHtmlEntities(matches[2]);
        if (Array.isArray(output[key])) {
            output[key].push(value);
        } else if (typeof output[key] !== 'undefined') {
            output[key] = [output[key], value];
        } else {
            output[key] = value;
        }
    }
    return output;
}

function first(value) {
    return Array.isArray(value) ? value[0] : value;
}

if (!fs.existsSync(pathToKernScores)) {
    console.error(`❌ ${pathToKernScores} does not exist. Run \`make kern\` in the project root first.`);
    process.exit(1);
}

if (!fs.existsSync(choraleInfoBinary)) {
    console.error(`❌ ${choraleInfoBinary} does not exist. Run \`make build\` in the project root first.`);
    process.exit(1);
}

const infosById = Object.fromEntries(
    JSON.parse(execFileSync(choraleInfoBinary, [pathToKernScores], { encoding: 'utf8', maxBuffer: 64 * 1024 * 1024 })).map((info) => [info.id, info]),
);

// Nuxt Content's dev server rebuilds its database for every file that changes.
function writeIfChanged(path, content) {
    if (fs.existsSync(path) && fs.readFileSync(path, 'utf8') === content) return false;
    fs.writeFileSync(path, content);
    return true;
}

if (!onlyChanged) fs.rmSync(choralesYamlPath, { recursive: true, force: true });
fs.mkdirSync(choralesYamlPath, { recursive: true });
const currentYamlFiles = new Set();
let changedCount = 0;

const files = fs.readdirSync(pathToKernScores).filter((file) => file.endsWith('.krn')).sort();

for (const file of files) {
    const id = basename(file, '.krn');

    const kern = fs.readFileSync(`${pathToKernScores}/${file}`, 'utf8');
    const referenceRecords = parseHumdrumReferenceRecords(kern);
    const info = infosById[id];

    const config = {
        choraleId: id,
        title: first(referenceRecords['OTL@@DE']) ?? null,
        composer: first(referenceRecords.COM) ?? null,
        bwv: first(referenceRecords.SCT) ?? null,
        number: parseInt(first(referenceRecords['PC#']), 10) || null,
        key: info?.key ?? null,
        mode: info?.mode ?? null,
        meters: info?.meters?.length ? info.meters : null,
        // The length in quarter notes, which the search results are ranked by.
        duration: info?.duration ?? null,
        phraseCount: info?.phraseCount ?? null,
        modulations: info?.modulations ?? null,
        cantusFirmus: info?.cantusFirmus ?? null,
    };

    // Missing values are left out instead of written as null, so the collection schema can mark
    // them optional.
    for (const name of Object.keys(config)) {
        if (config[name] === null) delete config[name];
    }

    const yaml = dump(config, {
        indent: 4,
        lineWidth: -1,
        sortKeys: true,
    });
    currentYamlFiles.add(`${id}.yaml`);
    if (writeIfChanged(`${choralesYamlPath}${id}.yaml`, yaml)) changedCount++;
}

for (const file of fs.readdirSync(choralesYamlPath)) {
    if (file.endsWith('.yaml') && !currentYamlFiles.has(file)) {
        fs.rmSync(`${choralesYamlPath}${file}`);
        changedCount++;
    }
}

console.log(`✅ Extract metadata for all ${files.length} chorales (${changedCount} files changed)`);
