import { execSync } from 'node:child_process';
import { dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import fs from 'node:fs';
import { getFiles, readFile } from '../utils/fs.mjs'; 
import { lineIsFermataEnd, resolveToken, tokenIsDataRecord } from '../utils/humdrum.mjs'; 
import yaml from 'js-yaml'; 
import { writeYaml } from '../utils/yaml.mjs';
import { v5 as uuidv5 } from 'uuid';

const UUID_NAMESPACE = 'abe966bb-81ba-4760-bdbd-6688243eb522';

const __dirname = dirname(fileURLToPath(import.meta.url));

function getIdFromFilePath(path) {
    return path.split(/[\\\/]/).pop().split('.')[0];
}

const yamlPath = `${__dirname}/../../content/bach-cadences`;
const kernPath = `${__dirname}/../../kern/bach-cadences`;

execSync(`rm -rf ${yamlPath}/*`);
execSync(`mkdir -p ${yamlPath}`);
execSync(`rm -rf ${kernPath}/*`);
execSync(`mkdir -p ${kernPath}`);

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
${cadenceUltima.replace(']', '')}
*-`;
    const stdout = execSync(`echo ${escapeShell(kern)} | degx`).toString();
    return stdout.split('\n')[2].split('\t')[1];
}

function getCadenceStartInitialLineIndex(lines) {
    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];
        if (tokenIsDataRecord(line) || line.startsWith('=')) {
            return i;
        }
    }
    return 0;
}

function getMeasureNumber(lineIndex, lines) {
    for (let i = lineIndex; i >= 0; i--) {
        const line = lines[i];
        if (line.startsWith('=')) {
            return parseInt(line.split('\t')[0].replaceAll(/\D/g, ''), 10);
        }
    }
    return null;
}

function getStartBeat(lineIndex, lines) {
    for (let i = lineIndex; i < lines.length; i++) {
        const line = lines[i];
        if (tokenIsDataRecord(line)) {
            const tokens = line.split('\t');
            return {
                beat: parseFloat(tokens[9]),
                measureBeat: parseFloat(tokens[10]),
            };
        }
    }
    return 0;
}

getFiles(`${__dirname}/../../bach-370-chorales/kern`).forEach(file => {
    const choraleId = getIdFromFilePath(file);
    console.log(choraleId);
    const key = getKey(choraleId);
    // fb -con3l => chor194-42 chor276-23 chor325-39
    // `beat` will remove the first line of the chorales that start withb `!!!!SEGMENT`
    const output = execSync(`cat ${__dirname}/../../bach-370-chorales/kern/${choraleId}.krn | fb -caim | fb -con3m | beat -ca | beat -a`).toString().trim();
    const lines = output.split('\n');
    let nextCadenceStartLineIndex = getCadenceStartInitialLineIndex(lines);
    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];
        const tokens = line.split('\t');
        if (lineIsFermataEnd(lines, i, [0, 3, 5, 7])) {

            // write kern file
            // add 2; one for line index to line number + one for missing first line with `!!!!SEGMENT` because of the beat program
            const startLine = nextCadenceStartLineIndex + 2;
            const endLine = i + 2;
            const kernScore = execSync(`cat ${__dirname}/../../bach-370-chorales/kern/${choraleId}.krn | myank -I -l ${startLine}-${endLine} --hide-ending`).toString().trim();
            const cadenceFilename = `${uuidv5(kernScore, UUID_NAMESPACE)}.krn`;
            fs.writeFileSync(`${kernPath}/${cadenceFilename}`, kernScore);

            // write yaml file
            const { beat: startBeat, measureBeat: startMeasureBeat} = getStartBeat(nextCadenceStartLineIndex, lines);
            const fb = tokens[2].replaceAll('~', '-');
            const endBeat = parseFloat(tokens[9]);
            const startMeasure = getMeasureNumber(nextCadenceStartLineIndex, lines);
            const endMeasure = getMeasureNumber(i, lines);
            const endMeasureBeat = parseFloat(tokens[10]);
            const id = `${choraleId}-${endBeat.toString().replace('.', '_')}`;
            const degree = getCadenceDegree(key, resolveToken(i, 0, lines));
            const data = {
                id,
                choraleId,
                fb,
                startLine,
                endLine,
                startBeat,
                endBeat,
                startMeasure,
                endMeasure,
                startMeasureBeat,
                endMeasureBeat,
                degree,
                filename: cadenceFilename,
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

            nextCadenceStartLineIndex = i + 1;
        }
    }
});
