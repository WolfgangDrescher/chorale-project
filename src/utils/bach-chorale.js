export class BachChorale {
    constructor(chorale) {
        this._chorale = chorale;
    }

    get id() {
        return this._chorale.id;
    }

    get nr() {
        return this._chorale.nr;
    }

    get title() {
        return this._chorale.title;
    }

    get localRawFile() {
        return `/bach-370-chorales/${this.id}.krn`;
    }

    get rawFile() {
        return this._chorale.rawFile;
    }

    get sourceFile() {
        return this._chorale.sourceFile;
    }

    get vhvHref() {
        return `https://verovio.humdrum.org/?file=${this.rawFile}`;
    }

    get key() {
        return this._chorale.key;
    }

    get isMajor() {
        return /[A-Z]/.test(this.key);
    }

    get isMinor() {
        return this.isMajor === false;
    }

    get cantusFirmusMint() {
        return this._chorale.cantusFirmusMint;
    }
}

export function createBachChorale(chorale) {
    return new BachChorale(chorale);
}

export function createBachChorales(chorales) {
    if (Array.isArray(chorales)) {
        return chorales.map(chorale => createBachChorale(chorale));
    }
    throw new Error('Cannot convert passsed argument to Chorale objects');
}
