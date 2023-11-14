import { execSync } from 'node:child_process';
import { dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { getFiles } from '../utils/fs.mjs'; 
import { writeYaml } from '../utils/yaml.mjs'; 
import { tokenIsDataRecord } from '../utils/humdrum.mjs'; 

const __dirname = dirname(fileURLToPath(import.meta.url));

function getIdFromFilePath(path) {
    return path.split(/[\\\/]/).pop().split('.')[0];
} 

const yamlPath = `${__dirname}/../../content/bach-outer-voices.yaml`;

function getBeat(token, id) {
    return parseFloat(token) || fixWrongBeats(id);
}

function fixWrongBeats(id) {
    if (id === 'chor081') return 1;
    if (id === 'chor113') return 1;
    if (id === 'chor123') return 2;
    if (id === 'chor346') return 3.5;
    return null;
}

const slices = [];

getFiles(`${__dirname}/../../bach-370-chorales/kern`).forEach(file => {
    const choraleId = getIdFromFilePath(file);
    console.log(choraleId);

    const output = execSync(`cat ${__dirname}/../../bach-370-chorales/kern/${choraleId}.krn | extractxx -k 1,4 | fb -ac | fb -ac --hint | beat -a | beat -ac`).toString().trim();
    const lines = output.split('\n');

    let isPhraseStart = true;
    for (let i = 0; i < lines.length; i++) {
        const line = lines[i];
        if (tokenIsDataRecord(line)) {
            const tokens = line.split('\t');
            // ignore line when both voices have a null token
            // and if there is no rest in one of the two voice (fb will be `.`)
            if ((tokens[0] !== '.' || tokens[3] !== '.') && tokens[1] !== '.') {
                const isPhraseEnd = tokens[0].includes(';') && tokens[3].includes(';');
                slices.push({
                    choraleId,
                    lineIndex: i + 1,// `beat` will remove the first line of the chorales that start withb `!!!!SEGMENT`
                    beat: getBeat(tokens[4], choraleId),
                    fb: tokens[1],
                    hint: tokens[2],
                    isPhraseStart,
                    isPhraseEnd,
                });
                if (isPhraseStart) isPhraseStart = false;
                if (isPhraseEnd) isPhraseStart = true;
            }
        }
    }
});

writeYaml(yamlPath, slices);
