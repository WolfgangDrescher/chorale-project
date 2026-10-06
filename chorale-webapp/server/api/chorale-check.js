const CHORALE_CHECK_BIN = '../chorale-search/build/chorale-check';

const MAX_SCORE_BYTES = 2 * 1024 * 1024;

const EXIT_CODE_ERRORS = {
    3: (message) => new ValidationError('The score could not be used', message),
    2: (message) => new InvalidArgumentError('The chorale-check tool received invalid command-line arguments', message),
};

function parseScoreData(value) {
    if (typeof value !== 'string' || value.trim() === '') {
        throw new ValidationError(
            'The request contains one or more validation errors',
            '"data" must be the score itself: **kern or MusicXML text, as a string',
        );
    }
    if (Buffer.byteLength(value, 'utf8') > MAX_SCORE_BYTES) {
        throw new ContentTooLargeError(
            'The score is too large',
            `"data" is larger than ${MAX_SCORE_BYTES / 1024 / 1024} MB`,
        );
    }
    return value;
}

// The voice ranges the notes can be measured against (see VoiceRanges.hpp), the first being the
// one to begin with.
const VOICE_RANGES = ['strauss-berlioz', 'bach'];

function parseVoiceRanges(value) {
    if (value === undefined || value === null || value === '') return VOICE_RANGES[0];
    if (!VOICE_RANGES.includes(value)) {
        throw new ValidationError(
            'The request contains one or more validation errors',
            `"voiceRanges" must be one of ${VOICE_RANGES.join(', ')}, got ${JSON.stringify(value)}`,
        );
    }
    return value;
}

// Takes { data, voiceRanges? } and returns the prepared four-voice kern and what the checks found
// in it. Answers with the JSON result, or as a stream of progress events ending in the result for
// a caller that accepts one (see respondWithStream).
export default defineEventHandler(async (event) => {
    setResponseHeader(event, 'Content-Type', 'application/json');

    try {
        const body = await parseRequestBody(event);
        const data = parseScoreData(body.data);
        const voiceRanges = parseVoiceRanges(body.voiceRanges);
        const check = ({ onEvent, signal } = {}) => {
            const args = ['-', '--voice-ranges', voiceRanges];
            if (onEvent) args.push('--progress');

            return runCliTool({
                bin: CHORALE_CHECK_BIN,
                toolName: 'chorale-check',
                args,
                input: data,
                exitCodeErrors: EXIT_CODE_ERRORS,
                onEvent,
                signal,
            });
        };
        const answer = ({ stdout, durationMs }) => {
            const { kern, findings } = parseToolJsonOutput(stdout, 'chorale-check');
            return { kern, findings, durationMs };
        };

        if (acceptsStream(event)) {
            return respondWithStream(event, async (send, signal) => answer(await check({ onEvent: send, signal })));
        }
        return answer(await check());
    } catch (e) {
        return toErrorResponse(event, e);
    }
});
