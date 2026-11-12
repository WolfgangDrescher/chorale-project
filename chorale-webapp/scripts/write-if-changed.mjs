import fs from 'node:fs';

// Nuxt Content's dev server rebuilds its database for every file that changes.
export function writeIfChanged(path, content) {
    if (fs.existsSync(path) && fs.readFileSync(path, 'utf8') === content) return false;
    fs.writeFileSync(path, content);
    return true;
}
