import fs from 'node:fs';
import yaml from 'js-yaml';

export function writeYaml(file, object, options) {
    const data = yaml.dump(object, Object.assign({
        indent: 4,
        lineWidth: -1,
        sortKeys: true,
    }, options || {}));
    fs.writeFileSync(file, data)
    return yaml.load();
}
