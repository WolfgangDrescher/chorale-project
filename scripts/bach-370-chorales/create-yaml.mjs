import { execSync } from 'node:child_process';
import { dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { decode } from 'html-entities';
import { getFiles, readFile } from '../utils/fs.mjs'; 
import { parseHumdrumReferenceRecords, tokenIsDataRecord } from '../utils/humdrum.mjs';
import { writeYaml } from '../utils/yaml.mjs';
import cliProgress from 'cli-progress';

console.log('Create YAML config for Bach chorales');

const progressBar = new cliProgress.SingleBar({
    format: ' {bar} {percentage}% | ETA: {eta}s | {value}/{total} | {id}',
}, cliProgress.Presets.shades_classic);

const __dirname = dirname(fileURLToPath(import.meta.url));

function getIdFromFilePath(path) {
    return path.split(/[\\\/]/).pop().split('.')[0];
}

const yamlPath = `${__dirname}/../../content/bach-370-chorales`;

execSync(`rm -rf ${yamlPath}/*`);
execSync(`mkdir -p ${yamlPath}`);

function getKey(file) {
    // get first occurence of key designations such as *E-:
    const stdout = execSync(`extractxx -f 1 ${file} | grep '^\\*\\([A-Ha-h]\\)\\([#-]*\\):'`).toString();
    const regex = new RegExp(/^\*([a-hA-H])([#-]*):(\w{3})?$/);
    const matches = regex.exec(stdout.trim());
    return matches ? `${matches[1]}${matches[2]}` : null;
}

function getCantusFirmusMelodicIntervalsQuarterNotesOnly(file) {
    // get melodic intervals of soprano voice but ignore other beats than quarter notes
    const output = execSync(`extractxx -f 4 ${file} | ridxx -d | beat -a | sed -E '/\\t[0-9]+\\.[0-9]+$/d' | extractxx -f 1 | ridxx -LGTM | mint -d | ridxx -I | sed -E '/^\\[/d'`).toString().trim();
    const mintLines = output.split('\n');
    const kernOutput = execSync(`extractxx -f 4 ${file} | ridxx -d | beat -a | sed -E '/\\t[0-9]+\\.[0-9]+$/d' | extractxx -f 1 | ridxx -LGTM | ridxx -I`).toString().trim();
    const kernLines = kernOutput.split('\n');
    for (let i = 0; i < kernLines.length; i++) {
        const line = kernLines[i];
        if (tokenIsDataRecord(line) && line.includes(';')) {
            mintLines[i - 1] = `${mintLines[i - 1]};`;
        }
    }
    return mintLines.join(',').replaceAll(';,', ';');
}

function getCantusFirmusMelodicIntervals(file) {
    // get melodic intervals of soprano voice
    const output = execSync(`extractxx -f 4 ${file} | ridxx -d | extractxx -f 1 | ridxx -LGTM | mint | ridxx -I | sed -E '/^\\[/d'`).toString().trim();
    const mintLines = output.split('\n');
    const kernOutput = execSync(`extractxx -f 4 ${file} | ridxx -d | extractxx -f 1 | ridxx -LGTM | ridxx -I`).toString().trim();
    const kernLines = kernOutput.split('\n');
    for (let i = 0; i < kernLines.length; i++) {
        const line = kernLines[i];
        if (tokenIsDataRecord(line) && line.includes(';')) {
            mintLines[i - 1] = `${mintLines[i-1]};`;
        }
    }
    return mintLines.join(',').replaceAll(';,', ';');
}

function getHarmonicIntervals(file) {
    const output = execSync(`cat ${file} | fb -caon --hint | extractxx -f 2 | ridxx -LGTM | ridxx -I`).toString().trim();
    const hintLines = output.split('\n');
    const kernOutput = execSync(`cat ${file} | extractxx -f 2 | ridxx -LGTM | ridxx -I`).toString().trim();
    const kernLines = kernOutput.split('\n');
    for (let i = 0; i < kernLines.length; i++) {
        const line = kernLines[i];
        if (tokenIsDataRecord(line) && line.includes(';')) {
            hintLines[i] = `${hintLines[i]};`;
        }
    }
    return hintLines.join(',').replaceAll(';,', ';');
}

function getNumberOfMeasures(file) {
    // get last measure number
    const stdout = execSync(`cat ${file}`).toString();
    const lines = stdout.trim().split('\n');
    for (let i = lines.length - 1; i >= 0; i--) {
        const line = lines[i];
        if (line.match(/^=\d/)) {
            return parseInt(line.split('\t')[0].replaceAll(/\D/g, ''), 10);
        }
    }
    return null;
}

function getTimeSignature(file) {
    const stdout = execSync(`cat ${file}`).toString();
    const lines = stdout.trim().split('\n');
    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];
        if (line.match(/^\*M[^M]/)) {
            return line.split('\t')[0].replace(/^\*M/, '');
        }
    }
    return null;
}

const files = getFiles(`${__dirname}/../../bach-370-chorales/kern`)

progressBar.start(files.length, 0);

files.forEach(file => {
    const id = getIdFromFilePath(file);
    progressBar.increment({ id });
    const yamlFile = `${yamlPath}/${id}.yaml`;
    const kern = readFile(file);
    const referenceRecords = parseHumdrumReferenceRecords(kern);

    const chorale = {};
    
    chorale.id = id;

    chorale.key = getKey(file);

    chorale.nr = parseInt(id.replace('chor', ''), 10);

    chorale.cantusFirmusMintQuarterNotes = getCantusFirmusMelodicIntervalsQuarterNotesOnly(file);

    chorale.cantusFirmusMint = getCantusFirmusMelodicIntervals(file);

    chorale.title = decode(referenceRecords['OTL@@DE']) || null;

    chorale.rawFile = `https://raw.githubusercontent.com/craigsapp/bach-370-chorales/master/kern/${id}.krn`;

    chorale.sourceFile = `https://github.com/craigsapp/bach-370-chorales/blob/master/kern/${id}.krn`;

    chorale.measures = getNumberOfMeasures(file);

    chorale.timeSignature = getTimeSignature(file);

    chorale.harmonicIntervals = getHarmonicIntervals(file);

    writeYaml(yamlFile, chorale);
    progressBar.increment();
});

progressBar.stop();
