
import { dirname } from 'node:path';
import { fileURLToPath } from 'node:url';
import { writeFileSync } from 'node:fs';
import { getFiles, readFile } from '../utils/fs.mjs';
import yaml from 'js-yaml'; 
import { createBachChorales } from '../../src/utils/bach-chorale.js';

const __dirname = dirname(fileURLToPath(import.meta.url));

const choralesData = getFiles(`${__dirname}/../../content/bach-370-chorales`).map(file => {
    return yaml.load(readFile(file));
});

const phrasesData = getFiles(`${__dirname}/../../content/bach-phrases`).map(file => {
    return yaml.load(readFile(file));
});

const chorales = createBachChorales(choralesData, phrasesData);

const keys = [...new Set(chorales.map(chorale => chorale.key).filter(n => n))];

const timeSignatures = [...new Set(chorales.map(chorale => chorale.timeSignature).filter(n => n))];

const degrees = [...new Set(chorales.map(chorale => chorale.phrases.map(c => c.degree)).flat().filter(n => n))]

const fbNumbers = [...new Set(chorales.map(chorale => chorale.phrases.map(c => c.fb)).flat().filter(n => n))]

const json = {
    keys,
    timeSignatures,
    degrees,
    fbNumbers,
};

console.log(json);

writeFileSync(`${__dirname}/../../src/utils/bach-chorale-filter-options.json`, JSON.stringify(json, null, '\t'), 'utf8');
