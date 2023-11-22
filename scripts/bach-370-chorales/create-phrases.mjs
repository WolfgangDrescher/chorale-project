import { execSync } from 'node:child_process';
import { dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import fs from 'node:fs';
import { getFiles, readFile } from '../utils/fs.mjs'; 
import { lineIsFermataEnd, resolveToken, tokenIsDataRecord } from '../utils/humdrum.mjs'; 
import yaml from 'js-yaml'; 
import { writeYaml } from '../utils/yaml.mjs';
import { v5 as uuidv5 } from 'uuid';
import cliProgress from 'cli-progress';

console.log('Create kern score and config for each phrase of the Bach chorales');

const progressBar = new cliProgress.SingleBar({
    format: ' {bar} {percentage}% | ETA: {eta}s | {value}/{total} | {id}',
}, cliProgress.Presets.shades_classic);

const UUID_NAMESPACE = 'abe966bb-81ba-4760-bdbd-6688243eb522';

const __dirname = dirname(fileURLToPath(import.meta.url));

function getIdFromFilePath(path) {
    return path.split(/[\\\/]/).pop().split('.')[0];
}

const yamlPath = `${__dirname}/../../content/bach-phrases`;
const kernPath = `${__dirname}/../../kern/bach-phrases`;

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
        if (line.startsWith('=') && line.match(/^=\d+/)) {
            return parseInt(line.split('\t')[0].replaceAll(/\D/g, ''), 10);
        }
    }
    return 0;
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

function getContinuationStartLine(beat, lines) {
    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];
        const tokens = line.split('\t');
        const lineBeat = parseFloat(tokens[9]);
        if (tokenIsDataRecord(line) && lineBeat >= beat - 2) {
            return i;
        }
    }
    return null; 
}

function getContinuationEndLine(beat, fermataDur, lines) {
    for (let i = lines.length - 1 ; i >= 0; i--) {
        const line = lines[i];
        const tokens = line.split('\t');
        const lineBeat = parseFloat(tokens[9]);
        if (tokenIsDataRecord(line) && lineBeat < beat + fermataDur + 2) {
            return i;
        }
    }
    return null;
}

function getContinuationLineIndex(lineIndex, fermataDur, lines) {
    const fermataBeatPos = parseFloat(lines[lineIndex].split('\t')[9]);
    if (lineIndex) {
        for (let i = lineIndex + 1; i < lines.length; i++) {
            const line = lines[i];
            const beat = parseFloat(line.split('\t')[9]);
            if (tokenIsDataRecord(line) && beat >= fermataDur + fermataBeatPos) {
                return i;
            }
        }
    }
    return null;
}

function getContinuationBassMint(endToken, startToken) {
    const kern = `**kern
${endToken}
${startToken}
*-`;
    const output = execSync(`echo ${escapeShell(kern)} | mint`).toString().trim();
    return output.split('\n')[2];
}

const files = getFiles(`${__dirname}/../../bach-370-chorales/kern`);

progressBar.start(files.length, 0);

files.forEach(file => {
    const choraleId = getIdFromFilePath(file);
    progressBar.increment({ id: choraleId });
    const key = getKey(choraleId);
    // fb -con3l => chor194-42 chor276-23 chor325-39
    // `beat` will remove the first line of the chorales that start withb `!!!!SEGMENT`
    const output = execSync(`cat ${__dirname}/../../bach-370-chorales/kern/${choraleId}.krn | fb -caim | fb -con3m | beat -ca | beat -a | beat -da`).toString().trim();
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
            let cadenceFilename = null;
            try {
                const kernScore = execSync(`cat ${__dirname}/../../bach-370-chorales/kern/${choraleId}.krn | myank -I -l ${startLine}-${endLine} --hide-ending`).toString().trim();
                const kernScoreFiltered = kernScore.split('\n').filter(l => !l.startsWith('!!xywh') && !l.startsWith('*>')).join('\n');
                cadenceFilename = `${uuidv5(kernScoreFiltered, UUID_NAMESPACE)}.krn`;
                fs.writeFileSync(`${kernPath}/${cadenceFilename}`, kernScoreFiltered);
            } catch (e) {
                console.error(`Error creating score for ${choraleId} lines ${startLine}-${endLine}`);
            }

            // write yaml file
            const { beat: startBeat, measureBeat: startMeasureBeat} = getStartBeat(nextCadenceStartLineIndex, lines);
            const fb = tokens[2].replaceAll('~', '-');
            const endBeat = parseFloat(tokens[9]);
            const startMeasure = getMeasureNumber(nextCadenceStartLineIndex, lines);
            const endMeasure = getMeasureNumber(i, lines);
            const endMeasureBeat = parseFloat(tokens[10]);
            const id = `${choraleId}-${endBeat.toString().replace('.', '_')}`;
            const degree = getCadenceDegree(key, resolveToken(i, 0, lines));
            const fermataDur = parseFloat(tokens[11]);

            const conLineIndex = getContinuationLineIndex(i, fermataDur, lines);
            let continuation = null;
            if (conLineIndex) {
                // write continuation kern file
                // add 2; one for line index to line number + one for missing first line with `!!!!SEGMENT` because of the beat program
                const conStartLine = getContinuationStartLine(endBeat, lines) + 2;
                const conEndLine = getContinuationEndLine(endBeat, fermataDur, lines) + 2;
                let conFilename = null;
                try {
                    const conKernScore = execSync(`cat ${__dirname}/../../bach-370-chorales/kern/${choraleId}.krn | myank -I -l ${conStartLine}-${conEndLine} --hide-ending`).toString().trim();
                    conFilename = `${uuidv5(conKernScore, UUID_NAMESPACE)}.krn`;
                    fs.writeFileSync(`${kernPath}/${conFilename}`, conKernScore);
                } catch(e) {
                    console.error(`Error creating score for ${choraleId} lines ${conStartLine}-${conEndLine}`);
                }
            
                const conFb = lines[conLineIndex].split('\t')[2].replaceAll('~', '-');
                const conDegree = getCadenceDegree(key, resolveToken(conLineIndex, 0, lines));
                const conBassMint = getContinuationBassMint(resolveToken(endLine - 2, 0, lines), resolveToken(conLineIndex, 0, lines));

                continuation = {
                    filename: conFilename,
                    fb: conFb,
                    degree: conDegree,
                    lineIndex: conLineIndex + 2,
                    mint: conBassMint,
                }
            }

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
                },
                continuation,
            }
            writeYaml(`${yamlPath}/${id}.yaml`, data);

            nextCadenceStartLineIndex = i + 1;
        }
    }
});

progressBar.stop();
