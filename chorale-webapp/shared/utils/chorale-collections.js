// Bach's chorales: the corpus that is searched, generated into kern/ by `make kern`.
export const BACH_CHORALES = {
    id: 'bach',
    name: 'Bach',
    directory: 'bach-370-chorales',
    repository: 'craigsapp/bach-370-chorales',
    branch: 'main',
};

// The collections shown besides Bach's chorales, one submodule each. They are not searched.
export const CHORALE_COLLECTIONS = [
    {
        id: 'kittel',
        name: 'Kittel',
        directory: 'kittel-24-chorales',
        repository: 'WolfgangDrescher/kittel-24-chorales',
        branch: 'master',
        titleRecord: 'OTL@@DE',
    },
    {
        id: 'schiorring',
        name: 'Schiørring',
        directory: 'schiorring-choral-bog',
        repository: 'WolfgangDrescher/schiorring-choral-bog',
        branch: 'master',
        titleRecord: 'OTL@@DA',
    },
];

export function findChoraleCollection(id) {
    return [BACH_CHORALES, ...CHORALE_COLLECTIONS].find((collection) => collection.id === id) ?? null;
}
