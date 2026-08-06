const CHORALE_SEARCH_BIN = '../chorale-search/build/chorale-search';

// The generated corpus (`make corpus`), carries the analysis spines already,
// which is what --no-analysis below relies on. Deriving them per request costs
// ~90ms per chorale and would blow the tool timeout on the full corpus.
const CORPUS_DIR = '../corpus/bach-370-chorales';

const EXIT_CODE_ERRORS = {
    3: (message) => new ValidationError('The search query contains one or more validation errors', message),
    2: (message) => new InvalidArgumentError('The chorale-search tool received invalid command-line arguments', message),
};

export default defineEventHandler(async (event) => {
    setResponseHeader(event, 'Content-Type', 'application/json');

    try {
        const body = await parseRequestBody(event);
        const { stdout, durationMs } = runCliTool({
            bin: CHORALE_SEARCH_BIN,
            toolName: 'chorale-search',
            args: [CORPUS_DIR, '--query', JSON.stringify(body), '--format', 'json', '--group-by-chorale', '--no-analysis'],
            exitCodeErrors: EXIT_CODE_ERRORS,
            overflowHint: 'Narrow the query, or cap it with "limit"',
        });
        return { results: parseToolJsonOutput(stdout, 'chorale-search'), durationMs };
    } catch (e) {
        return toErrorResponse(event, e);
    }
});
