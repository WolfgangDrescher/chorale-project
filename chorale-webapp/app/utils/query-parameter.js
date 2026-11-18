// ?query= as useQueryLink writes it: JSON or base64url-encoded JSON.
export function parseQueryParameter(value) {
    try {
        return JSON.parse(value);
    } catch {}
    try {
        const bytes = Uint8Array.from(atob(value.replaceAll('-', '+').replaceAll('_', '/')), (c) => c.charCodeAt(0));
        return JSON.parse(new TextDecoder('utf-8', { fatal: true }).decode(bytes));
    } catch {
        return undefined;
    }
}
