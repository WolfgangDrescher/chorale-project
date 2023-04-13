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
execSync(`mkdir ${yamlPath}`);

getFiles(`${__dirname}/../../bach-370-chorales/kern`).forEach(file => {
    const id = getIdFromFilePath(file);
    const yamlFile = `${yamlPath}/${id}.yaml`;
    console.log(id);
    const kern = readFile(file);
    const referenceRecords = parseHumdrumReferenceRecords(kern);

    const chorale = {};
    
    chorale.id = id;

    chorale.nr = parseInt(id.replace('chor', ''), 10);

    chorale.title = decode(referenceRecords['OTL@@DE']) || null;

    chorale.rawFile = `https://raw.githubusercontent.com/craigsapp/bach-370-chorales/master/kern/${id}.krn`;

    chorale.sourceFile = `https://github.com/craigsapp/bach-370-chorales/blob/master/kern/${id}.krn`;

    writeYaml(yamlFile, chorale);
});
