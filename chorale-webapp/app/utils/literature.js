const DEPENDENT_TYPES = ['article', 'incollection', 'unpublished'];
const listFormat = new Intl.ListFormat('de', { type: 'conjunction' });
const givenNameFirst = (name) => name.split(', ').reverse().join(' ');

// One entry of a bibliography as segments: [{ text, italic?, href? }]
export function formatLiteratureEntry(entry, formatDate) {
    const names = (list) => listFormat.format(list.map(givenNameFirst));
    const author = entry.author && listFormat.format(entry.author.map((name, index) => (index ? givenNameFirst(name) : name)));
    const container = entry.booktitle ?? entry.journal;
    const edition = typeof entry.edition === 'number' ? `${entry.edition}. Auflage` : entry.edition;
    const parts = [
        DEPENDENT_TYPES.includes(entry.type) ? [{ text: `«${entry.title}»` }] : [{ text: entry.title, italic: true }],
        container && [{ text: 'in: ' }, { text: container, italic: true }, entry.journal && { text: ` ${entry.volume} (${entry.year})` }],
        entry.editor && [{ text: `hrsg. von ${names(entry.editor)}` }],
        entry.translator && [{ text: `übers. von ${names(entry.translator)}` }],
        ...[!entry.journal && entry.volume && `Bd. ${entry.volume}`, entry.series, edition, entry.type === 'phdthesis' && 'Dissertation', entry.school, entry.note]
            .map((text) => text && [{ text }]),
        (entry.address || entry.publisher) && [{ text: [entry.address, entry.publisher].filter(Boolean).join(': ') }],
        !entry.journal && entry.year && [{ text: String(entry.year) }],
        entry.pages && [{ text: `S. ${entry.pages}` }],
        entry.url && [{ text: 'URL: ' }, { text: entry.url, href: entry.url }, entry.urldate && { text: ` (besucht am ${formatDate(entry.urldate)})` }],
    ].filter(Boolean);
    return [
        author && { text: `${author}: ` },
        ...parts.flatMap((part, index) => [index && { text: ', ' }, ...part]),
        { text: '.' },
    ].filter(Boolean);
}
