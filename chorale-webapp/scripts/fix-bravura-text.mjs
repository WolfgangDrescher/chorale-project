import { createHash } from 'node:crypto';
import fs from 'node:fs';
import { dirname, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';

// @fontsource/bravura-text ships Bravura Text 1.380, whose cmap Chrome rejects, so its file is
// replaced by the one of Bravura 1.482. Remove once fixed: fontsource/fontsource#1048.
const URL = 'https://raw.githubusercontent.com/steinbergmedia/bravura/bravura-1.482/redist/woff/BravuraText.woff2';
const SHA256 = '1f2711e9b554b7240edadc48edc2bece1a8b91118c6825fe7ff03ed1e07e1574';
const target = resolve(dirname(fileURLToPath(import.meta.url)), '../node_modules/@fontsource/bravura-text/files/bravura-text-latin-400-normal.woff2');

const sha256Of = (buffer) => createHash('sha256').update(buffer).digest('hex');

if (sha256Of(fs.readFileSync(target)) !== SHA256) {
    const response = await fetch(URL);
    if (!response.ok) throw new Error(`${URL}: ${response.status}`);
    const font = Buffer.from(await response.arrayBuffer());
    if (sha256Of(font) !== SHA256) throw new Error(`${URL}: unexpected SHA-256`);
    fs.writeFileSync(target, font);
}
