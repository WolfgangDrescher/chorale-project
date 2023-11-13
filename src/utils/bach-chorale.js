export class BachChorale {
    constructor(chorale, phrases) {
        this._chorale = chorale;
        this._phrases = phrases;
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

    get fullTitle() {
        return `${this.nr}. ${this.title}`;
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

    get measures() {
        return this._chorale.measures;
    }

    get timeSignature() {
        return this._chorale.timeSignature;
    }

    get phrases() {
        if (!this._phrases) throw new Error(`${this.id} does not have phrases assigned.`);
        return this._phrases;
    }

    get countPhrases() {
        return this.phrases.length;
    }
}

export function createBachChorale(chorale, phrases) {
    return new BachChorale(chorale, phrases);
}

export function createBachChorales(chorales, phrases) {
    if (Array.isArray(chorales)) {
        return chorales.map(chorale => {
            const cadences = phrases?.filter(e => e.choraleId === chorale.id);
            return createBachChorale(typeof toRaw !== 'undefined' ? toRaw(chorale) : chorale, typeof toRaw !== 'undefined' ? toRaw(cadences) : cadences);
        });
    }
    throw new Error('Cannot convert passsed argument to Chorale objects');
}
