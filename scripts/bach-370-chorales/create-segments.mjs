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

function getIdFromFilePath(path) {
    return path.split(/[\\\/]/).pop().split('.')[0];
}

const yamlPath = `${__dirname}/../../content/bach-segments`;
const kernPath = `${__dirname}/../../kern/bach-segments`;

execSync(`rm -rf ${yamlPath}`);
execSync(`mkdir -p ${yamlPath}`);
execSync(`rm -rf ${kernPath}`);
execSync(`mkdir -p ${kernPath}`);

const files = getFiles(`${__dirname}/../../bach-370-chorales/kern`)

progressBar.start(files.length, 0);

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

const betaNums = [4];

const tmp = [];
files.forEach(file => {
    const id = getIdFromFilePath(file);
    progressBar.increment({ id });
    betaNums.forEach(beatNum => {
        const output = execSync(`cat ${__dirname}/../../bach-370-chorales/kern/${id}.krn | beat -ca -A 0`).toString().trim();
        const lines = output.split('\n');
        // lines.forEach((l, i) => console.log(i, l))
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
                            const kernScore = execSync(`cat ${__dirname}/../../bach-370-chorales/kern/${id}.krn | myank -I -l ${startLine}-${endLine} --hide-ending`).toString().trim();
                            const filename = `${uuidv5(kernScore, UUID_NAMESPACE)}.krn`;
                            fs.writeFileSync(`${kernPath}/${filename}`, kernScore);
                            tmp.push(filename);
                        } catch (e) {
                            console.error(`Error creating score for ${id} lines ${startLine}-${endLine}`);
                        }
                    }
                }
            }
        }
    });
});
progressBar.stop();
console.log(tmp);
writeYaml(`${yamlPath}/segments.yaml`, tmp);
