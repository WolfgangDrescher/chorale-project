import { execFileSync } from 'node:child_process';

const EXEC_FILE_SYNC_TIMEOUT = 10_000;

// Node caps a child's stdout at 1 MB by default, which a broad query over the full corpus
// blows past -- matching every note is ~16 MB of JSON.
const EXEC_FILE_SYNC_MAX_BUFFER = 5 * 1024 * 1024;

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

// Runs a binary and returns its stdout plus durationMs. `input` goes to stdin, which is how an
// uploaded score travels: never onto the filesystem, never into argv. `exitCodeErrors` maps an
// exit code to a factory taking the tool's "Error: ..." line; codes outside it become 500s.
export function runCliTool({ bin, toolName, args = [], input, exitCodeErrors = {}, overflowHint,
    timeout = EXEC_FILE_SYNC_TIMEOUT }) {
    const startedAt = performance.now();
    try {
        const stdout = execFileSync(bin, args, {
            encoding: 'utf8',
            timeout,
            maxBuffer: EXEC_FILE_SYNC_MAX_BUFFER,
            input,
        });
        return { stdout, durationMs: Math.round(performance.now() - startedAt) };
    } catch (e) {
        if (e.code === 'ENOBUFS') {
            throw new ResponseTooLargeError(
                `The ${toolName} tool returned more than ${EXEC_FILE_SYNC_MAX_BUFFER / 1024 / 1024} MB of results`,
                overflowHint,
            );
        }
        if (e.signal === 'SIGTERM' && e.status === null) {
            throw new ToolTimeoutError(`The ${toolName} tool timed out after ${timeout / 1000} seconds`);
        }
        if (typeof e.status !== 'number') {
            throw new ServiceUnavailableError(`The ${toolName} tool could not be started`);
        }
        const message = parseCliErrorMessage(e.stderr?.toString());
        const toError = exitCodeErrors[e.status];
        if (toError) throw toError(message);
        throw new ToolFailedError(`The ${toolName} tool exited with code ${e.status}`, message);
    }
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
export function toErrorResponse(event, e) {
    setResponseStatus(event, e.statusCode ?? 500);
    return {
        name: e.name,
        message: e.message,
        errors: e.errors ? (Array.isArray(e.errors) ? e.errors : [e.errors]) : [],
    };
}
