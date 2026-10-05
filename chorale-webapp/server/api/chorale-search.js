const CHORALE_SEARCH_BIN = '../chorale-search/build/chorale-search';

// The generated corpus (`make corpus`), carries the analysis spines already,
// which is what --no-analysis below relies on. Deriving them per request costs
// ~90ms per chorale and would blow the tool timeout on the full corpus.
const CORPUS_DIR = '../corpus/bach-370-chorales';

const EXIT_CODE_ERRORS = {
    3: (message) => new ValidationError('The search query contains one or more validation errors', message),
    2: (message) => new InvalidArgumentError('The chorale-search tool received invalid command-line arguments', message),
};

// Answers with the JSON result, or as a stream of progress events ending in the result for a
// caller that accepts one (see respondWithStream).
export default defineEventHandler(async (event) => {
    setResponseHeader(event, 'Content-Type', 'application/json');

    try {
        const body = await parseRequestBody(event);
        const search = ({ onEvent, signal } = {}) => {
            const args = [
                CORPUS_DIR,
                '--query', JSON.stringify(body),
                '--format', 'json',
                '--group-by-chorale',
                '--no-analysis',
            ];
            if (onEvent) args.push('--progress');

            return runCliTool({
                bin: CHORALE_SEARCH_BIN,
                toolName: 'chorale-search',
                args,
                exitCodeErrors: EXIT_CODE_ERRORS,
                overflowHint: 'Narrow the query, or cap it with "limit"',
                onEvent,
                signal,
            });
        };
        const answer = ({ stdout, durationMs }) => ({ results: parseToolJsonOutput(stdout, 'chorale-search'), durationMs });

        if (acceptsStream(event)) {
            return respondWithStream(event, async (send, signal) => answer(await search({ onEvent: send, signal })));
        }
        return answer(await search());
    } catch (e) {
        return toErrorResponse(event, e);
    }
});
