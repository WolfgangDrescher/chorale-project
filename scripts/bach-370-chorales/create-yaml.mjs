import { execSync } from 'node:child_process';
import { dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { decode } from 'html-entities';
import { getFiles, readFile } from '../utils/fs.mjs'; 
import { parseHumdrumReferenceRecords } from '../utils/humdrum.mjs';
import { writeYaml } from '../utils/yaml.mjs';

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

function getCantusFirmusMelodicIntervals(file) {
    // get melodic intervals of soprano voice but ignore other beats than quater notes
    const output = execSync(`extractxx -f 4 ${file} | ridxx -d | beat -a | sed -E '/\\t[0-9]+\\.[0-9]+$/d' | extractxx -f 1 | ridxx -LGTM | mint -d | ridxx -I | sed -E '/^\\[/d'`).toString().trim();
    return output.split('\n').join(',');
}

getFiles(`${__dirname}/../../bach-370-chorales/kern`).forEach(file => {
    const id = getIdFromFilePath(file);
    console.log(id);
    const yamlFile = `${yamlPath}/${id}.yaml`;
    const kern = readFile(file);
    const referenceRecords = parseHumdrumReferenceRecords(kern);

    const chorale = {};
    
    chorale.id = id;

    chorale.key = getKey(file);

    chorale.nr = parseInt(id.replace('chor', ''), 10);

    chorale.cantusFirmusMint = getCantusFirmusMelodicIntervals(file);

    chorale.title = decode(referenceRecords['OTL@@DE']) || null;

    chorale.rawFile = `https://raw.githubusercontent.com/craigsapp/bach-370-chorales/master/kern/${id}.krn`;

    chorale.sourceFile = `https://github.com/craigsapp/bach-370-chorales/blob/master/kern/${id}.krn`;

    writeYaml(yamlFile, chorale);
});
