import fs from 'node:fs';
import { basename, dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import yaml from 'js-yaml';

const __dirname = dirname(fileURLToPath(import.meta.url));

// The corpus `make kern` generates in the project root, or another directory given as the first
// argument (e.g. the plain bach-370-chorales/kern sources).
const pathToKernScores = resolve(process.argv[2] ?? `${__dirname}/../../kern/bach-370-chorales`);
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

// The meter the chorale starts in, e.g. `*M3/4`.
function parseMeter(humdrum) {
    return humdrum.match(/^\*M(\d+\/\d+)/m)?.[1] ?? null;
}

// Humdrum's abbreviations of the modes that come after the colon of a key (`*a:dor`).
const MODES = {
    ion: 'ionian',
    dor: 'dorian',
    phr: 'phrygian',
    lyd: 'lydian',
    mix: 'mixolydian',
    aeo: 'aeolian',
    loc: 'locrian',
};

// The key the chorale starts in, e.g. `*G:` or `*a:dor` for a mode, as a line all spines share.
function parseKey(humdrum) {
    for (const line of humdrum.split(/\r?\n/)) {
        const tokens = line.split('\t');
        const match = tokens[0].match(/^\*([a-gA-G][#-]*):([a-z]*)$/);
        if (match && tokens.every((token) => token === tokens[0])) {
            // Without a mode label the case of the tonic says major or minor.
            const mode = MODES[match[2]] ?? (match[1] === match[1].toLowerCase() ? 'minor' : 'major');
            return { key: match[1], mode };
        }
    }
    return { key: null, mode: null };
}

function first(value) {
    return Array.isArray(value) ? value[0] : value;
}

if (!fs.existsSync(pathToKernScores)) {
    console.error(`❌ ${pathToKernScores} does not exist. Run \`make kern\` in the project root first.`);
    process.exit(1);
}

fs.rmSync(choralesYamlPath, { recursive: true, force: true });
fs.mkdirSync(choralesYamlPath, { recursive: true });

const files = fs.readdirSync(pathToKernScores).filter((file) => file.endsWith('.krn')).sort();

for (const file of files) {
    const id = basename(file, '.krn');
    console.log(`✅ Extract metadata for ${id}`);

    const kern = fs.readFileSync(`${pathToKernScores}/${file}`, 'utf8');
    const referenceRecords = parseHumdrumReferenceRecords(kern);
    const { key, mode } = parseKey(kern);

    const config = {
        choraleId: id,
        title: first(referenceRecords['OTL@@DE']) ?? null,
        composer: first(referenceRecords.COM) ?? null,
        bwv: first(referenceRecords.SCT) ?? null,
        number: parseInt(first(referenceRecords['PC#']), 10) || null,
        key,
        mode,
        meter: parseMeter(kern),
    };

    // Missing values are left out instead of written as null, so the collection schema can mark
    // them optional.
    for (const name of Object.keys(config)) {
        if (config[name] === null) delete config[name];
    }

    fs.writeFileSync(`${choralesYamlPath}${id}.yaml`, yaml.dump(config, {
        indent: 4,
        lineWidth: -1,
        sortKeys: true,
    }));
}
