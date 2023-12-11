export class SchiorringChorale {
    constructor(chorale) {
        this.chorale = chorale;
    }

    get id() {
        return this.chorale.id;
    }

    get nr() {
        return this.chorale.nr;
    }

    get title() {
        return this.chorale.title;
    }

    get localRawFile() {
        return `/kern/schiorring-choral-bog/${this.id}.krn`;
    }

    get rawFile() {
        return this.chorale.rawFile;
    }

    get sourceFile() {
        return this.chorale.sourceFile;
    }

    get vhvHref() {
        return `https://verovio.humdrum.org/?file=${this.rawFile}`;
    }
}

export function createSchiorringChorale(chorale) {
    return new SchiorringChorale(chorale);
}

export function createSchiorringChorales(chorales) {
    if (Array.isArray(chorales)) {
        return chorales.map(chorale => createSchiorringChorale(chorale));
    }
    throw new Error('Cannot convert passsed argument to Chorale objects');
}
