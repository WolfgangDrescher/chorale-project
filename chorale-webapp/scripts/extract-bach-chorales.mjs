import { execFileSync } from 'node:child_process';
import fs from 'node:fs';
import { basename, dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { dump } from 'js-yaml';
import { parseHumdrumReferenceRecords } from './humdrum-reference-records.mjs';
import { writeIfChanged } from './write-if-changed.mjs';

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
const choralesYamlPath = `${__dirname}/../content/bach-chorales/`;

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
        urlScan: first(referenceRecords['URL-scan'])?.split(/\s/)[0] ?? null,
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
