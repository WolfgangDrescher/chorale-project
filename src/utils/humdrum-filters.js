import { v4 as uuidv4 } from 'uuid';
import { default as tailwindColors } from 'tailwindcss/colors';

function createColorList() {
    return [
        'red',
        'blue',
        'green',
        'violet',
        'orange',
        'sky',
        'emerald',
        'purple',
        'amber',
        'fuchsia',
    ].map(name => tailwindColors[name][500]);
}

function createMatchedNoteList() {
    const list = [
        // https://github.com/humdrum-tools/verovio-humdrum-viewer/issues/782
        // 'i', 'j', 'l',
        'N', 'V', 'Z', '@', '+', '|', '<', '>',
    ];

    let i;
    for (i = 0; i < 50; i++) {
        list.push(String.fromCodePoint(`0x1F6${i.toString().padStart(2, '0')}`));
    }
    return list;
}

class Line {
    constructor(value, linePrefix) {
        this.value = value || '';
        this.prefix = linePrefix || '!!!filter: ';
    }

    toString() {
        return `${this.prefix}${this.value}`;
    }
}

export class HumdrumFilter {
    static NAME = 'HumdrumFilter';
    id = null;
    unique = false;
    changeable = false;
    priority = 0;
    lines = [];
    configurable = false;

    constructor() {
        this.id = uuidv4();
    }

    get className() {
        return this.constructor.NAME;
    }

    addLine(value, prefix) {
        this.lines.push(new Line(value, prefix));
    }

    beforeRemove() {}

    toString() {
        return this.lines.map(l => l.toString()).join('\n');
    }

    static colors = createColorList();
    static usedColors = [];
    static getNextColor() {
        let color = null;
        HumdrumFilter.colors.some(c => {
            const isUsed = HumdrumFilter.usedColors.includes(c);
            if (!isUsed) {
                color = c;
            }
            return !isUsed;
        });
        if (color) {
            HumdrumFilter.usedColors.push(color);
            return color;
        }
        throw new Error('Cannot create next color');
    }

    static chars = createMatchedNoteList();
    static usedChars = [];
    static getNextMatchedNoteChar() {
        let char = null;
        CintFilter.chars.some(c => {
            const isUsed = CintFilter.usedChars.includes(c);
            if (!isUsed) {
                char = c;
            }
            return !isUsed;
        });
        if (char) {
            CintFilter.usedChars.push(char);
            return char;
        }
        throw new Error('Cannot create an unused char for matched notes mapping');
    }
}

export class Satb2gsFilter extends HumdrumFilter {
    static NAME = 'Satb2gsFilter';
    priority = 1;
    unique = true;
    lines = [
        new Line('satb2gs'),
    ];
}

export class IntervallsatzPresetFilter extends HumdrumFilter {
    priority = -1;
    static NAME = 'IntervallsatzPresetFilter';
    unique = true;
    lines = [
        new Line('fb -icatm'),
    ];
}

export class FiguredbassPresetFilter extends HumdrumFilter {
    static NAME = 'FiguredbassPresetFilter';
    unique = true;
    lines = [
        new Line('fb -acron3'),
    ];
}

export class ExtendedFiguredbassPresetFilter extends HumdrumFilter {
    static NAME = 'ExtendedFiguredbassPresetFilter';
    unique = true;
    lines = [
        new Line('fb -acon3'),
    ];
}

export class BassScaleDegreeFilter extends HumdrumFilter {
    static NAME = 'BassScaleDegreeFilter';
    unique = true;
    lines = [
        new Line('deg --circle -k 1'),
    ];
}

export class ScaleDegreePresetFilter extends HumdrumFilter {
    static NAME = 'ScaleDegreePresetFilter';
    unique = true;
    lines = [
        new Line('deg --circle'),
    ];
}

export class HideMiddleVoicesFilter extends HumdrumFilter {
    static NAME = 'HideMiddleVoicesFilter';
    priority = 2;
    unique = true;
    lines = [
        new Line('extract -f 1,$'),
    ];
}

export class ExtractSpineFilter extends HumdrumFilter {
    static NAME = 'ExtractSpineFilter';
    priority = 2;
    unique = true;
    changeable = true;
    configurable = true;

    constructor(value) {
        super();
        this.value = value;
        this.addLine(`extract -f ${this.value}`);
    }
}

export class HideFiguredbassFilter extends HumdrumFilter {
    static NAME = 'HideFiguredbassFilter';
    unique = true;
    lines = [
        new Line('extract -I **fb'),
        new Line('extract -I **fba'),
    ];
}

export class FiguresWithoutSlashesFilter extends HumdrumFilter {
    static NAME = 'FiguresWithoutSlashesFilter';
    unique = true;
    lines = [
        new Line('shed -X **fb -e "s/slash/Xslash/I"'),
    ];
}
