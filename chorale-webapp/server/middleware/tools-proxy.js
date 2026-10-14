// The endpoints that run a command line tool of chorale-search.
const TOOL_PATHS = new Set(['/api/chorale-search', '/api/chorale-segment', '/api/chorale-check']);

// The request headers that matter to those endpoints, and the response headers
// to hand back.
const FORWARDED_REQUEST_HEADERS = ['content-type', 'accept'];
const FORWARDED_RESPONSE_HEADERS = ['content-type', 'cache-control', 'x-accel-buffering'];

// With CHORALE_TOOLS_URL set (the Docker setup), the tools run in the tools
// service and these requests are passed on to it as they are. Without it the
// endpoints run the tools themselves, which is what the development server
// does.
export default defineEventHandler(async (event) => {
    const toolsUrl = process.env.CHORALE_TOOLS_URL;
    const path = event.path.split('?')[0];
    if (!toolsUrl || !TOOL_PATHS.has(path)) return;

    // Stops the tool in the tools service when the client goes away, as it does
    // when the tool runs here. Not the request's own "close": that is emitted
    // as soon as its body has been read.
    const res = event.node.res;
    const abort = new AbortController();
    res.on('close', () => {
        if (!res.writableFinished) abort.abort();
    });

    try {
        const upstream = await fetch(new URL(event.path, toolsUrl), {
            method: event.method,
            headers: Object.fromEntries(FORWARDED_REQUEST_HEADERS
                .map((name) => [name, getHeader(event, name)])
                .filter(([, value]) => value)),
            body: await readRawBody(event, false),
            signal: abort.signal,
        });
        setResponseStatus(event, upstream.status);
        for (const name of FORWARDED_RESPONSE_HEADERS) {
            const value = upstream.headers.get(name);
            if (value) setResponseHeader(event, name, value);
        }
        // The body is handed on as it arrives, which is what makes the progress
        // events reach the client.
        return upstream.body;
    } catch (e) {
        if (abort.signal.aborted) return;
        return toErrorResponse(event, new ServiceUnavailableError('The tools service could not be reached'));
    }
});
