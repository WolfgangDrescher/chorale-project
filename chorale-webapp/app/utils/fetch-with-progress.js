// POSTs `body` to an endpoint that streams its progress (see respondWithStream on the server)
// and resolves with the final result. Every event before it goes to `onProgress`. A failure,
// as the response's status or as an error event in the stream, is thrown like ofetch throws:
// `statusCode`, and `data` with { name, message, errors }. Aborting `signal` ends the request
// (and, on the server, the tool) and rejects with an AbortError.
export async function fetchWithProgress(url, { body, onProgress, signal }) {
    const response = await fetch(url, {
        method: 'POST',
        headers: { 'content-type': 'application/json', accept: 'application/x-ndjson' },
        body: typeof body === 'string' ? body : JSON.stringify(body),
        signal,
    });
    if (!response.ok) {
        const data = await response.json().catch(() => ({}));
        throw Object.assign(new Error(data.message ?? response.statusText), { statusCode: response.status, data });
    }

    const reader = response.body.pipeThrough(new TextDecoderStream()).getReader();
    let buffered = '';
    let result = null;

    function handleLine(line) {
        if (!line.trim()) return;
        const { event, ...rest } = JSON.parse(line);
        if (event === 'result') {
            result = rest;
        } else if (event === 'error') {
            const { statusCode, ...data } = rest;
            throw Object.assign(new Error(data.message), { statusCode, data });
        } else {
            onProgress?.({ event, ...rest });
        }
    }

    for (;;) {
        const { value, done } = await reader.read();
        if (done) break;
        buffered += value;
        let newline;
        while ((newline = buffered.indexOf('\n')) !== -1) {
            handleLine(buffered.slice(0, newline));
            buffered = buffered.slice(newline + 1);
        }
    }
    handleLine(buffered);

    if (!result) throw new Error('The connection closed before the result arrived');
    return result;
}
