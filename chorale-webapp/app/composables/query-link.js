// Turns a query into a link to a page that runs it right away, the search unless `routeName`
// names another. Valid JSON is compacted and base64url-encoded, which survives apps that turn
// %2B back into a + and is shorter than percent-encoded JSON. Anything else is passed as it is
// for the page to report.
export function useQueryLink(routeName = 'search') {
    const localePath = useLocalePath();
    return (query) => {
        try {
            query = toBase64Url(JSON.stringify(JSON.parse(query)));
        } catch {}
        return localePath({ name: routeName, query: { query } });
    };
}

function toBase64Url(text) {
    const binary = String.fromCharCode(...new TextEncoder().encode(text));
    return btoa(binary).replaceAll('+', '-').replaceAll('/', '_').replace(/=+$/, '');
}
