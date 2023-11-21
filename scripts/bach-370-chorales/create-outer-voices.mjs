import { execSync } from 'node:child_process';
import { dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { getFiles } from '../utils/fs.mjs'; 
import { writeYaml } from '../utils/yaml.mjs'; 
import { tokenIsDataRecord, getResolvedTokenLineIndex } from '../utils/humdrum.mjs'; 
import cliProgress from 'cli-progress';

console.log('Create outer voices of the Bach chorales');

const progressBar = new cliProgress.SingleBar({
    format: ' {bar} {percentage}% | ETA: {eta}s | {value}/{total} | {id}',
}, cliProgress.Presets.shades_classic);

const __dirname = dirname(fileURLToPath(import.meta.url));

function getIdFromFilePath(path) {
    return path.split(/[\\\/]/).pop().split('.')[0];
} 

const yamlPath = `${__dirname}/../../content/bach-outer-voices.yaml`;

function getBeat(token, id) {
    return parseFloat(token);
}

const slices = [];

const files = getFiles(`${__dirname}/../../bach-370-chorales/kern`);

progressBar.start(files.length, 0);

files.forEach(file => {
    const choraleId = getIdFromFilePath(file);
    progressBar.increment({ id: choraleId });

    const output = execSync(`cat ${__dirname}/../../bach-370-chorales/kern/${choraleId}.krn | beat -a | extractxx -f 1,4,5 | fb -ac | fb -ac --hint`).toString().trim();
    const lines = output.split('\n');

    const choraleSlices = [];

    let isPhraseStart = true;
    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];
        if (tokenIsDataRecord(line)) {
            const tokens = line.split('\t');
            // ignore line when both voices have a null token
            // and if there is no rest in one of the two voice (fb will be `.`)
            if ((tokens[0] !== '.' || tokens[3] !== '.') && tokens[1] !== '.') {
                const bassLineIndex = getResolvedTokenLineIndex(i, 0, lines);
                const sopranoLineIndex = getResolvedTokenLineIndex(i, 3, lines);
                const isPhraseEnd = lines[bassLineIndex].split('\t')[0].includes(';') && lines[sopranoLineIndex].split('\t')[3].includes(';');
                choraleSlices.push({
                    choraleId,
                    lineIndex: i + 1,// `beat` will remove the first line of the chorales that start withb `!!!!SEGMENT`
                    beat: getBeat(tokens[4], choraleId),
                    fb: tokens[1],
                    hint: tokens[2],
                    isPhraseStart,
                    isPhraseEnd,
                    isChoraleStart: false,
                    isChoraleEnd: false,
                });
                if (isPhraseStart) isPhraseStart = false;
                if (isPhraseEnd) isPhraseStart = true;
            }
        }
    }
    choraleSlices[0].isChoraleStart = true;
    choraleSlices[choraleSlices.length - 1].isChoraleEnd = true;
    slices.push(...choraleSlices);
});

writeYaml(yamlPath, slices);

progressBar.stop();
