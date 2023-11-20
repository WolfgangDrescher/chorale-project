import { execSync } from 'node:child_process';
import { dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { decode } from 'html-entities';
import { getFiles, readFile } from '../utils/fs.mjs'; 
import { parseHumdrumReferenceRecords } from '../utils/humdrum.mjs';
import { writeYaml } from '../utils/yaml.mjs';
import cliProgress from 'cli-progress';

console.log('Create YAML config for Kittel chorales');

const progressBar = new cliProgress.SingleBar({
    format: ' {bar} {percentage}% | ETA: {eta}s | {value}/{total} | {id}',
}, cliProgress.Presets.shades_classic);

const __dirname = dirname(fileURLToPath(import.meta.url));

function getIdFromFilePath(path) {
    return path.split(/[\\\/]/).pop().split('.')[0];
}

const yamlPath = `${__dirname}/../../content/kittel-24-chorales`;

execSync(`rm -rf ${yamlPath}/*`);
execSync(`mkdir -p ${yamlPath}`);

const files = getFiles(`${__dirname}/../../kittel-24-chorales/kern`);

progressBar.start(files.length, 0);

files.forEach(file => {
    const id = getIdFromFilePath(file);
    progressBar.increment({ id });

    const yamlFile = `${yamlPath}/${id}.yaml`;
    const kern = readFile(file);
    const referenceRecords = parseHumdrumReferenceRecords(kern);

    const chorale = {};
    
    chorale.id = id;

    chorale.nr = parseInt(id.replace('chor', ''), 10);

    chorale.title = decode(referenceRecords['OTL@@DE']) || null;

    chorale.rawFile = `https://raw.githubusercontent.com/WolfgangDrescher/kittel-24-chorales/master/kern/${id}.krn`;

    chorale.sourceFile = `https://github.com/WolfgangDrescher/kittel-24-chorales/blob/master/kern/${id}.krn`;

    writeYaml(yamlFile, chorale);
});

progressBar.stop();
