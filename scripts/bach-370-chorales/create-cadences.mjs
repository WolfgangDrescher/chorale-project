import { execSync } from 'node:child_process';
import { dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { getFiles, readFile } from '../utils/fs.mjs'; 
import { lineIsFermataEnd, resolveToken } from '../utils/humdrum.mjs'; 
import yaml from 'js-yaml'; 
import { writeYaml } from '../utils/yaml.mjs';

const __dirname = dirname(fileURLToPath(import.meta.url));

function getIdFromFilePath(path) {
    return path.split(/[\\\/]/).pop().split('.')[0];
}

const yamlPath = `${__dirname}/../../content/bach-cadences`;

execSync(`rm -rf ${yamlPath}/*`);
execSync(`mkdir -p ${yamlPath}`);

function escapeShell(cmd) {
    return '"' + cmd.replace(/(["$`\\])/g, '\\$1') + '"';
}

function getKey(id) {
    const choraleYamlFile = `${__dirname}/../../content/bach-370-chorales/${id}.yaml`;
    const config = yaml.load(readFile(choraleYamlFile));
    return config.key;
}

function getCadenceDegree(key, cadenceUltima) {
    const kern = `**kern
*${key}:
${cadenceUltima.toUpperCase()}
*-`;
    const stdout = execSync(`echo ${escapeShell(kern)} | deg`).toString();
    return stdout.split('\n')[2];
}

getFiles(`${__dirname}/../../bach-370-chorales/kern`).forEach(file => {
    const choraleId = getIdFromFilePath(file);
    console.log(choraleId);
    const key = getKey(choraleId);
    // fb -con3l => chor194-42 chor276-23 chor325-39
    const output = execSync(`cat ${__dirname}/../../bach-370-chorales/kern/${choraleId}.krn | fb -cai | fb -con3 | beat -ca`).toString();
    const lines = output.trim().split('\n');

    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];
        const tokens = line.split('\t');
        if (lineIsFermataEnd(lines, i, [0, 3, 5, 7])) {
            const fb = tokens[2];
            const beat = parseFloat(tokens[9]);
            const id = `${choraleId}-${beat.toString().replace('.', '_')}`;
            const degree = getCadenceDegree(key, resolveToken(i, 0, lines));
            const data = {
                id,
                choraleId,
                fb,
                beat,
                degree,
                voices: {
                    soprano: {
                        interval: tokens[8],
                    },
                    alto: {
                        interval: tokens[6],
                    },
                    tenor: {
                        interval: tokens[4],
                    },
                    bass: {
                        interval: null,
                    },
                }
            }
            writeYaml(`${yamlPath}/${id}.yaml`, data);
        }
    }
});
