import { spawn } from 'node:child_process';

const DEFAULT_TIMEOUT = 10_000;

// A broad query over the full corpus blows past any small cap -- matching every note is
// ~16 MB of JSON -- so what a tool may print is limited here.
const MAX_STDOUT_BYTES = 5 * 1024 * 1024;

// The shape of every error below: an HTTP status, and a `name` spelled out rather than read
// off the constructor, which a production build is free to rename.
class ApiError extends Error {
    constructor(name, statusCode, message, errors) {
        super(message);
        this.name = name;
        this.statusCode = statusCode;
        this.errors = errors;
    }
}

// There was no usable JSON body to begin with, so nothing has been validated yet.
export class InvalidRequestError extends ApiError {
    constructor(message, errors) {
        super('InvalidRequestError', 400, message, errors);
    }
}

// The request body is past the cap -- an oversized upload. 413 is about the request alone, so
// the remedy is to send less; the response-side counterpart below is a different situation.
export class ContentTooLargeError extends ApiError {
    constructor(message, errors) {
        super('ContentTooLargeError', 413, message, errors);
    }
}

// The body parsed, and what it says cannot be acted on: a query the tool rejects, a segment
// length that isn't one. 422 rather than 400 -- the syntax was fine, the content was not.
export class ValidationError extends ApiError {
    constructor(message, errors) {
        super('ValidationError', 422, message, errors);
    }
}

// The request was fine and its answer outgrew what the endpoint hands back. Deliberately not
// 413: nothing is wrong with the request's size, and shrinking it is not the remedy -- the
// query has to ask for less, which is what the accompanying hint says.
export class ResponseTooLargeError extends ApiError {
    constructor(message, errors) {
        super('ResponseTooLargeError', 422, message, errors);
    }
}

// The command line was built on this side, so a tool rejecting it is a bug here -- 500 even
// though the message reads like a validation error.
export class InvalidArgumentError extends ApiError {
    constructor(message, errors) {
        super('InvalidArgumentError', 500, message, errors);
    }
}

// The tool ran and failed in a way this endpoint has no reading for: an exit code outside its
// table. 502, because the failure is upstream of us and not in the request.
export class ToolFailedError extends ApiError {
    constructor(message, errors) {
        super('ToolFailedError', 502, message, errors);
    }
}

// The tool exited successfully and wrote something that isn't the JSON it promised -- the
// textbook bad gateway: an upstream answer this side cannot use.
export class InvalidToolOutputError extends ApiError {
    constructor(message, errors) {
        super('InvalidToolOutputError', 502, message, errors);
    }
}

// The tool could not be started at all: the binary is missing or not executable, so the corpus
// tooling this endpoint fronts is not deployed. Nothing about the request is wrong.
export class ServiceUnavailableError extends ApiError {
    constructor(message, errors) {
        super('ServiceUnavailableError', 503, message, errors);
    }
}

// The tool started and did not finish inside its timeout, so there is no answer to pass on.
export class ToolTimeoutError extends ApiError {
    constructor(message, errors) {
        super('ToolTimeoutError', 504, message, errors);
    }
}

// The CLIs print their failures as one "Error: ..." line on stderr. Matched with its colon,
// because the libraries underneath grumble in near-misses ("Error on line: 1:") that would
// otherwise win by coming first.
export function parseCliErrorMessage(stderrText) {
    const line = (stderrText ?? '')
        .split('\n')
        .map((line) => line.trim())
        .find((line) => line.startsWith('Error:'));

    return line ? line.replace(/^Error:\s*/, '') : undefined;
}

// The body every endpoint here needs: each of them requires one, so an empty body is as much a
// failure as an unparseable one.
export async function parseRequestBody(event) {
    try {
        const body = await readBody(event);
        if (!body) throw new InvalidRequestError('JSON is empty');
        return body;
    } catch (e) {
        if (e instanceof InvalidRequestError) throw e;
        throw new InvalidRequestError('Invalid JSON body');
    }
}

// A line of the tool's stderr that is one of its --progress events: a JSON object with an
// "event" key. Everything else it says there (its "Error: ..." line, a library's grumbling)
// is not one.
function parseProgressEvent(line) {
    if (!line.startsWith('{')) return null;
    try {
        const event = JSON.parse(line);
        return typeof event?.event === 'string' ? event : null;
    } catch {
        return null;
    }
}

// Runs a binary and resolves with its stdout plus durationMs. `input` goes to stdin, which is
// how an uploaded score travels: never onto the filesystem, never into argv. `exitCodeErrors`
// maps an exit code to a factory taking the tool's "Error: ..." line; other codes become
// ToolFailedError. `onEvent` gets each progress event the tool writes while it runs, and
// `signal` stops the tool early (a client that went away). The tool runs asynchronously, so a
// long run does not hold up the server.
export function runCliTool({ bin, toolName, args = [], input, exitCodeErrors = {}, overflowHint,
    timeout = DEFAULT_TIMEOUT, onEvent, signal }) {
    const startedAt = performance.now();
    return new Promise((resolve, reject) => {
        const child = spawn(bin, args);
        let stdout = '';
        let stdoutBytes = 0;
        let partialLine = '';
        const stderrLines = [];
        let tooLarge = false;
        let timedOut = false;
        let settled = false;

        const settle = (finish, value) => {
            if (settled) return;
            settled = true;
            clearTimeout(timer);
            finish(value);
        };
        const timer = setTimeout(() => {
            timedOut = true;
            child.kill();
        }, timeout);
        signal?.addEventListener('abort', () => child.kill(), { once: true });

        child.stdout.setEncoding('utf8');
        child.stdout.on('data', (chunk) => {
            stdoutBytes += Buffer.byteLength(chunk);
            if (stdoutBytes > MAX_STDOUT_BYTES) {
                tooLarge = true;
                child.kill();
                return;
            }
            stdout += chunk;
        });

        const handleStderrLine = (line) => {
            const progressEvent = parseProgressEvent(line);
            if (progressEvent) onEvent?.(progressEvent);
            else if (line.trim()) stderrLines.push(line);
        };
        child.stderr.setEncoding('utf8');
        child.stderr.on('data', (chunk) => {
            partialLine += chunk;
            let newline;
            while ((newline = partialLine.indexOf('\n')) !== -1) {
                handleStderrLine(partialLine.slice(0, newline));
                partialLine = partialLine.slice(newline + 1);
            }
        });

        // The tool may exit without reading all of its input; that is its exit code's to say.
        child.stdin.on('error', () => {});
        child.stdin.end(input);

        child.on('error', () => {
            settle(reject, new ServiceUnavailableError(`The ${toolName} tool could not be started`));
        });
        child.on('close', (code) => {
            if (partialLine) handleStderrLine(partialLine);
            if (tooLarge) {
                settle(reject, new ResponseTooLargeError(
                    `The ${toolName} tool returned more than ${MAX_STDOUT_BYTES / 1024 / 1024} MB of results`,
                    overflowHint,
                ));
            } else if (timedOut) {
                settle(reject, new ToolTimeoutError(`The ${toolName} tool timed out after ${timeout / 1000} seconds`));
            } else if (code === 0) {
                settle(resolve, { stdout, durationMs: Number((performance.now() - startedAt).toFixed(3)) });
            } else {
                const message = parseCliErrorMessage(stderrLines.join('\n'));
                const toError = exitCodeErrors[code];
                settle(reject, toError
                    ? toError(message)
                    : new ToolFailedError(`The ${toolName} tool exited with code ${code}`, message));
            }
        });
    });
}

// The tool exited successfully; its stdout has to be the JSON it promised.
export function parseToolJsonOutput(stdout, toolName) {
    try {
        return JSON.parse(stdout);
    } catch (e) {
        throw new InvalidToolOutputError(`The ${toolName} tool returned output that could not be parsed as JSON`);
    }
}

// The single answer shape every endpoint fails with. `errors` is always an array, so the
// frontend can list it blind; a plain Error arrives here as a 500 with none.
function errorBody(e) {
    return {
        name: e.name,
        message: e.message,
        errors: e.errors ? (Array.isArray(e.errors) ? e.errors : [e.errors]) : [],
    };
}

export function toErrorResponse(event, e) {
    setResponseStatus(event, e.statusCode ?? 500);
    return errorBody(e);
}

// Whether the caller accepts the answer as a stream (see respondWithStream).
export function acceptsStream(event) {
    return (getHeader(event, 'accept') ?? '').includes('application/x-ndjson');
}

// Answers as a stream of newline-delimited JSON (application/x-ndjson): whatever `work` sends
// while it runs (the tool's progress events), then one last line, either
// { event: 'result', ...what work returned } or { event: 'error', statusCode, ...the usual
// error body }. The status is 200 by then, so a failure travels as that last line. A client
// that disconnects aborts `work`'s signal, which stops the tool.
export function respondWithStream(event, work) {
    setResponseHeader(event, 'Content-Type', 'application/x-ndjson');
    setResponseHeader(event, 'Cache-Control', 'no-cache');
    setResponseHeader(event, 'X-Accel-Buffering', 'no'); // keeps nginx from holding the stream back

    const abort = new AbortController();
    const encoder = new TextEncoder();
    return new ReadableStream({
        async start(controller) {
            const send = (line) => {
                if (!abort.signal.aborted) controller.enqueue(encoder.encode(`${JSON.stringify(line)}\n`));
            };
            try {
                send({ event: 'result', ...(await work(send, abort.signal)) });
            } catch (e) {
                send({ event: 'error', statusCode: e.statusCode ?? 500, ...errorBody(e) });
            }
            if (!abort.signal.aborted) controller.close();
        },
        cancel() {
            abort.abort();
        },
    });
}
