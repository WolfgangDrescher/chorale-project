import { execSync } from 'node:child_process';
import { dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import fs from 'node:fs';
import { getFiles } from '../utils/fs.mjs';
import { writeYaml } from '../utils/yaml.mjs';
import cliProgress from 'cli-progress';
import { v5 as uuidv5 } from 'uuid';
import { lineIsFermataEnd, lineIsRestsOnly, tokenIsDataRecord } from '../utils/humdrum.mjs'; 

console.log('Create segments for Bach chorales');

const progressBar = new cliProgress.SingleBar({
    format: ' {bar} {percentage}% | ETA: {eta}s | {value}/{total} | {id}',
}, cliProgress.Presets.shades_classic);

const UUID_NAMESPACE = 'abe966bb-81ba-4760-bdbd-6688243eb522';

const __dirname = dirname(fileURLToPath(import.meta.url));

const yamlPath = `${__dirname}/../../content/bach-segments`;
const kernPath = `${__dirname}/../../kern/bach-segments`;

execSync(`rm -rf ${yamlPath}`);
execSync(`mkdir -p ${yamlPath}`);
execSync(`rm -rf ${kernPath}`);
execSync(`mkdir -p ${kernPath}`);

function getIdFromFilePath(path) {
    return path.split(/[\\\/]/).pop().split('.')[0];
}

function escapeShell(cmd) {
    return '"' + cmd.replace(/(["$`\\])/g, '\\$1') + '"';
}

function getEndLineIndex(lineIndex, lines, startBeat, beatNum) {
    let endLineIndex = null;
    for (let i = lineIndex + 1; i < lines.length; i++) {
        const line = lines[i];
        if (tokenIsDataRecord(line)) {
            const tokens = line.split('\t');
            const lineBeat = parseFloat(tokens[4]);
            if (lineBeat >= startBeat + beatNum) {
                break;
            }
            endLineIndex = i;
        }
    }
    if (endLineIndex) {
        const endBeat = lines[endLineIndex].split('\t')[4];
        // exclude when segments are shorter than beatNum without the duration
        // of the last line
        if (Math.floor(endBeat - startBeat) !== beatNum - 1) {
            return null;
        }
        for (let ii = lineIndex; ii < endLineIndex; ii++) {
            // exclude segments with fermatas in the middle
            if (lineIsFermataEnd(lines, ii, [0,1,2,3])) {
                return null;
            }
            // exclude segments with rests in the middle
            if (lineIsRestsOnly(lines, ii, [0, 1, 2, 3])) {
                return null;
            }
        }
    }
    return endLineIndex;
}

const files = getFiles(`${__dirname}/../../bach-370-chorales/kern`)

const beatNums = [3, 4];

progressBar.start(files.length * beatNums.length, 0);

beatNums.forEach(beatNum => {
    const data = [];
    files.forEach(file => {
        const choraleId = getIdFromFilePath(file);
        progressBar.increment({ id: `${choraleId}/${beatNum}-beats` });
        const output = execSync(`cat ${__dirname}/../../bach-370-chorales/kern/${choraleId}.krn | beat -ca -A 0`).toString().trim();
        const lines = output.split('\n');
        for (let i = 0; i < lines.length; i++) {
            const line = lines[i];
            const tokens = line.split('\t');
            if (tokenIsDataRecord(line)) {
                const lineBeat = parseFloat(tokens[4]);
                if (lineBeat % 1 === 0) {
                    const endLineIndex = getEndLineIndex(i, lines, lineBeat, beatNum);
                    if (endLineIndex) {
                        const startLine = i + 2;
                        const endLine = endLineIndex + 2;
                        try {
                            const kernScore = execSync(`cat ${__dirname}/../../bach-370-chorales/kern/${choraleId}.krn | myank -l ${startLine}-${endLine} --hide-ending --hide-starting`).toString().trim();
                            const filename = `${uuidv5(kernScore, UUID_NAMESPACE)}.krn`;
                            fs.writeFileSync(`${kernPath}/${filename}`, kernScore);

                            const bassMintData = execSync(`echo ${escapeShell(kernScore)} | extractxx -f 1 | mint -c`).toString().trim();
                            const sopranoMintData = execSync(`echo ${escapeShell(kernScore)} | extractxx -f 4 | mint -c`).toString().trim();
                            const fbData = execSync(`echo ${escapeShell(kernScore)} | fb -cn --hint | fb -c -k 1,4 --hint | beat -ca -A 0`).toString().trim();
                            const kern = execSync(`paste -d '\t' <(echo ${escapeShell(fbData)}) <(echo ${escapeShell(bassMintData)}) <(echo ${escapeShell(sopranoMintData)}) | extractxx -f 2,3,7,8,9`, { shell: '/bin/bash' }).toString().trim();
                            const slices = [];

                            kern.split('\n').forEach((l) => {
                                if (tokenIsDataRecord(l)) {
                                    const tokens = l.split('\t');
                                    const fb = tokens[0];
                                    const fbOuterVoices = tokens[1];
                                    const beat = parseFloat(tokens[2]);
                                    const bassMint = tokens[3];
                                    const sopranoMint = tokens[4];
                                    // slices.push({
                                    //     fb: tokens[0],
                                    //     fbOuterVoices: tokens[1],
                                    //     beat: parseFloat(tokens[2]),
                                    //     bassMint: tokens[3],
                                    //     sopranoMint: tokens[4],
                                    // });
                                    slices.push([fb, fbOuterVoices, beat, bassMint, sopranoMint]);
                                }
                            });

                            // data.push({
                            //     choraleId,
                            //     filename,
                            //     startLine,
                            //     endLine,
                            //     slices,
                            // });
                            data.push([choraleId, filename, startLine, endLine, slices])
                        } catch (e) {
                            console.error(`Error creating score for ${choraleId} lines ${startLine}-${endLine}`);
                        }
                    }
                }
            }
        }
    });
    // writeYaml(`${yamlPath}/${beatNum}-beats.yaml`, data);
    fs.writeFileSync(`${yamlPath}/${beatNum}-beats.json`, JSON.stringify(data), 'utf8');

});
progressBar.stop();
