const INDENT = '    ';
const MAX_LINE_LENGTH = 80;

// A complete query has a feature and a pattern. Snippets of a pattern, of a single step or of
// options alone do not.
export function isCompleteQuery(value) {
    return value !== null && typeof value === 'object' && 'feature' in value && 'pattern' in value;
}

// A query as in the docs: each option of a query and each step of a pattern on a line of its
// own, any other value on one line as long as it fits.
export function formatQuery(value) {
    return format(value, '', '');
}

function inline(value) {
    if (Array.isArray(value)) return `[${value.map(inline).join(', ')}]`;
    if (value === null || typeof value !== 'object') return JSON.stringify(value);
    const entries = Object.entries(value).map(([key, entry]) => `${JSON.stringify(key)}: ${inline(entry)}`);
    return entries.length ? `{ ${entries.join(', ')} }` : '{}';
}

function format(value, indent, key) {
    const prefix = key ? `${JSON.stringify(key)}: ` : '';
    const oneLine = inline(value);
    const mustExpand = isCompleteQuery(value) || (Array.isArray(value) && (key === 'pattern' || value.some(isCompleteQuery)));
    const fits = indent.length + prefix.length + oneLine.length + 1 <= MAX_LINE_LENGTH;
    if (value === null || typeof value !== 'object' || (!mustExpand && fits)) return prefix + oneLine;

    const inner = indent + INDENT;
    const lines = Array.isArray(value)
        ? value.map((item) => inner + format(item, inner, ''))
        : Object.entries(value).map(([entryKey, entry]) => inner + format(entry, inner, entryKey));
    if (!lines.length) return prefix + oneLine;
    const [open, close] = Array.isArray(value) ? ['[', ']'] : ['{', '}'];
    return `${prefix}${open}\n${lines.join(',\n')}\n${indent}${close}`;
}
