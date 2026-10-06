const CHORALE_SEGMENT_BIN = '../chorale-search/build/chorale-segment';

// Generated corpus (`make corpus`) with the analysis spines already in it.
const CORPUS_DIR = '../corpus/bach-370-chorales';

// The stats run searches every segment query over the whole corpus, which takes ~30s.
const SEGMENT_TIMEOUT = 60_000;

const MAX_SCORE_BYTES = 2 * 1024 * 1024;

const DEFAULT_SEGMENT_LENGTH = 4;

const EXIT_CODE_ERRORS = {
    3: (message) => new ValidationError('The score could not be used', message),
    2: (message) => new InvalidArgumentError('The chorale-segment tool received invalid command-line arguments', message),
};

// A bad length is the caller's mistake (422), not a bad command line of ours (500).
function parseSegmentLength(value) {
    if (value === undefined || value === null || value === '') return DEFAULT_SEGMENT_LENGTH;

    const length = Number(value);
    if (!Number.isInteger(length) || length <= 0) {
        throw new ValidationError(
            'The request contains one or more validation errors',
            `"length" takes a positive whole number of quarter notes, got ${JSON.stringify(value)}`,
        );
    }
    return length;
}

// A boolean option of the page. Absent means on, the default of every one of them.
function parseBoolean(name, value) {
    if (value === undefined || value === null) return true;
    if (typeof value !== 'boolean') {
        throw new ValidationError(
            'The request contains one or more validation errors',
            `"${name}" must be a boolean, got ${JSON.stringify(value)}`,
        );
    }
    return value;
}

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

// Takes { data, length?, ignoreIntervalQuality?, skipUnclassifiedBeats? } and returns the prepared kern and the segments with their corpus stats.
// Answers with the JSON result, or as a stream of progress events ending in the result for a
// caller that accepts one (see respondWithStream).
export default defineEventHandler(async (event) => {
    setResponseHeader(event, 'Content-Type', 'application/json');

    try {
        const body = await parseRequestBody(event);
        const data = parseScoreData(body.data);
        const length = parseSegmentLength(body.length);
        const ignoreIntervalQuality = parseBoolean('ignoreIntervalQuality', body.ignoreIntervalQuality);
        const skipUnclassifiedBeats = parseBoolean('skipUnclassifiedBeats', body.skipUnclassifiedBeats);
        const segment = ({ onEvent, signal } = {}) => {
            const args = [
                '-',
                '--length', String(length),
                '--stats', CORPUS_DIR,
                '--no-analysis',
            ];
            args.push('--mint-ignore-quality', String(ignoreIntervalQuality));
            args.push('--metweight-skip-unclassified', String(skipUnclassifiedBeats));
            if (onEvent) args.push('--progress');

            return runCliTool({
                bin: CHORALE_SEGMENT_BIN,
                toolName: 'chorale-segment',
                args,
                input: data,
                exitCodeErrors: EXIT_CODE_ERRORS,
                overflowHint: 'Segment with a longer "length"',
                timeout: SEGMENT_TIMEOUT,
                onEvent,
                signal,
            });
        };
        const answer = ({ stdout, durationMs }) => {
            const { inputFormat, layout, kern, segments } = parseToolJsonOutput(stdout, 'chorale-segment');
            return { inputFormat, layout, kern, segments, length, durationMs };
        };

        if (acceptsStream(event)) {
            return respondWithStream(event, async (send, signal) => answer(await segment({ onEvent: send, signal })));
        }
        return answer(await segment());
    } catch (e) {
        return toErrorResponse(event, e);
    }
});
