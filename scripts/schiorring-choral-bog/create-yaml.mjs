import { execSync } from 'node:child_process';
import { dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { decode } from 'html-entities';
import { getFiles, readFile } from '../utils/fs.mjs'; 
import { parseHumdrumReferenceRecords } from '../utils/humdrum.mjs';
import { writeYaml } from '../utils/yaml.mjs';
import cliProgress from 'cli-progress';

console.log('Create YAML config for Schiørring chorales');

const progressBar = new cliProgress.SingleBar({
    format: ' {bar} {percentage}% | ETA: {eta}s | {value}/{total} | {id}',
}, cliProgress.Presets.shades_classic);

const __dirname = dirname(fileURLToPath(import.meta.url));

function getIdFromFilePath(path) {
    return path.split(/[\\\/]/).pop().split('.')[0];
}

const yamlPath = `${__dirname}/../../content/schiorring-choral-bog`;

execSync(`rm -rf ${yamlPath}/*`);
execSync(`mkdir -p ${yamlPath}`);

const files = getFiles(`${__dirname}/../../schiorring-choral-bog/kern`);

progressBar.start(files.length, 0);

files.forEach(file => {
    const id = getIdFromFilePath(file);
    progressBar.increment({ id });

    const yamlFile = `${yamlPath}/${id}.yaml`;
    const kern = readFile(file);
    const referenceRecords = parseHumdrumReferenceRecords(kern);

    const chorale = {};
    
    chorale.id = id;

    chorale.nr = id.substring(0, id.indexOf('-')).replace(/^0*|/, '');

    chorale.title = decode(referenceRecords['OTL@@DA']) || null;

    chorale.rawFile = `https://raw.githubusercontent.com/WolfgangDrescher/schiorring-choral-bog/master/kern/${id}.krn`;

    chorale.sourceFile = `https://github.com/WolfgangDrescher/schiorring-choral-bog/blob/master/kern/${id}.krn`;

    writeYaml(yamlFile, chorale);
});

progressBar.stop();
