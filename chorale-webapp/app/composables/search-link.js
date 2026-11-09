// Turns a query into a link to the search page, which searches it right away. Valid JSON is
// compacted and base64url-encoded, which survives apps that turn %2B back into a + and is
// shorter than percent-encoded JSON. Anything else is passed as it is for the search to report.
export function useSearchLink() {
    const localePath = useLocalePath();
    return (query) => {
        try {
            query = toBase64Url(JSON.stringify(JSON.parse(query)));
        } catch {}
        return localePath({ name: 'search', query: { query } });
    };
}

function toBase64Url(text) {
    const binary = String.fromCharCode(...new TextEncoder().encode(text));
    return btoa(binary).replaceAll('+', '-').replaceAll('/', '_').replace(/=+$/, '');
}
