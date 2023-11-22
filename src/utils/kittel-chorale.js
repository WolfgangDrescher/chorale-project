export class KittelChorale {
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
        return `/kern/kittel-24-chorales/${this.id}.krn`;
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

export function createKittelChorale(chorale) {
    return new KittelChorale(chorale);
}

export function createKittelChorales(chorales) {
    if (Array.isArray(chorales)) {
        return chorales.map(chorale => createKittelChorale(chorale));
    }
    throw new Error('Cannot convert passsed argument to Chorale objects');
}
