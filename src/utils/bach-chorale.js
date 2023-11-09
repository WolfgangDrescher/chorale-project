export class BachChorale {
    constructor(chorale, cadences) {
        this._chorale = chorale;
        this._cadencesItem = cadences;
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

export function createBachChorale(chorale, cadences) {
    return new BachChorale(chorale, cadences);
}

export function createBachChorales(chorales, choralCadences) {
    if (Array.isArray(chorales)) {
        return chorales.map(chorale => {
            const cadences = choralCadences?.find(e => e.choraleId === chorale.id);
            return createBachChorale(toRaw(chorale), toRaw(cadences));
        });
    }
    throw new Error('Cannot convert passsed argument to Chorale objects');
}
