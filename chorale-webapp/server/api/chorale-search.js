import { execFileSync } from 'node:child_process';

const CHORALE_SEARCH_BIN = '../chorale-search/build/chorale-search';
const CORPUS_DIR = '../kern/bach-370-chorales';

class ServiceUnavailableError extends Error {
    constructor(message, errors) {
        super(message);
        this.statusCode = 500;
        this.errors = errors;
    }
}
class ValidationError extends Error {
    constructor(message, errors) {
        super(message);
        this.statusCode = 400;
        this.errors = errors;
    }
}

class InvalidQueryError extends Error {
    constructor(message, errors) {
        super(message);
        this.statusCode = 400;
        this.errors = errors;
    }
}

class InvalidArgumentError extends Error {
    constructor(message, errors) {
        super(message);
        this.statusCode = 500;
        this.errors = errors;
    }
}

class InvalidSearchOutputError extends Error {
    constructor(message, errors) {
        super(message);
        this.statusCode = 500;
        this.errors = errors;
    }
}

function parseCliErrorMessage(stderrText) {
	const line = (stderrText ?? '')
		.split('\n')
		.map((line) => line.trim())
		.find((line) => line.startsWith('Error'));

	return line ? line.replace(/^Error:\s*/, '') : undefined;
}

async function parseRequestBody(event) {
	try {
		const body = await readBody(event);
		if (!body) throw new InvalidQueryError('JSON is empty');
		return body;
	} catch (e) {
		if (e instanceof InvalidQueryError) throw e;
		throw new InvalidQueryError('Invalid JSON body');
	}
}

function runChoraleSearch(body) {
    try {
        return execFileSync(CHORALE_SEARCH_BIN, [
            CORPUS_DIR,
            '--query',
            JSON.stringify(body),
            '--format',
            'json',
            '--group-by-chorale',
        ], { encoding: 'utf8' });
    } catch (e) {
        if (typeof e.status !== 'number') {
            throw new ServiceUnavailableError('The chorale-search tool could not be started');
        }
        const message = parseCliErrorMessage(e.stderr?.toString());
        switch (e.status) {
            case 3: throw new ValidationError('The search query contains one or more validation errors', message);
            case 2: throw new InvalidArgumentError('The chorale-search tool received invalid command-line arguments', message);
        }
        throw new Error(message);
    }
}

function parseSearchOutput(stdout) {
    try {
        return JSON.parse(stdout);
    } catch (e) {
        throw new InvalidSearchOutputError('The chorale-search tool returned output that could not be parsed as JSON');
    }
}

export default defineEventHandler(async (event) => {
    await new Promise(resolve => setTimeout(resolve, 1000));
    setResponseHeader(event, 'Content-Type', 'application/json');

    try {
        const body = await parseRequestBody(event);
        const stdout = runChoraleSearch(body);
        return parseSearchOutput(stdout);
    } catch (e) {
        setResponseStatus(event, e.statusCode ?? 500);
        return {
            name: e.constructor.name,
            message: e.message,
            errors: e.errors ? (Array.isArray(e.errors) ? e.errors : [e.errors]) : [],
        };
    }
});
