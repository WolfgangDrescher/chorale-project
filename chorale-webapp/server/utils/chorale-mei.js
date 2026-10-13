import { readFile } from 'node:fs/promises';
import { basename } from 'node:path';
import { VerovioToolkit } from 'verovio/esm';
import createVerovioModule from 'verovio/wasm-hum';

// What a download can hold of a chorale:
//   layout   score (the voices on staves of their own) or piano (a piano system)
export const CHORALE_EXPORT_LAYOUTS = ['score', 'piano'];

// The generated kern files, which the page serves as public assets (see nuxt.config.ts).
const KERN_DIR = '../kern/bach-370-chorales';

// The basename keeps an id from pointing anywhere else than at a kern file of that directory.
export function loadChoraleKern(id) {
    return readFile(`${KERN_DIR}/${basename(id)}.krn`, 'utf8');
}

// A note token (not a rest, not a null token) with its duration, tie, beam and slur marks
// stripped from the front.
const NOTE_TOKEN = /^[\[\(\{_]*(\d+(?:%\d+)?\.*)(?=.*[a-gA-G])/;

function isDataLine(line) {
    return line !== '' && !/^[!*=]/.test(line);
}

// The duration of a **kern token in quarter notes (0 for a grace note).
function tokenQuarters(token) {
    if (/q/i.test(token)) return 0;
    const match = token.match(/^[\[\(\{_]*(\d+)(?:%(\d+))?(\.*)/);
    if (!match) return 0;
    const [, reciprocal, numerator = '1', dots] = match;
    return 4 * Number(numerator) / Number(reciprocal) * (2 - 2 ** -dots.length);
}

// A duration in quarter notes as a **kern reciprocal ("2.", "4"), or null where no single
// note value (with up to two dots) has it.
function quartersToToken(quarters) {
    for (const reciprocal of [1, 2, 4, 8, 16, 32]) {
        for (let dots = 0; dots <= 2; dots++) {
            if (Math.abs(4 / reciprocal * (2 - 2 ** -dots) - quarters) < 1e-9) return `${reciprocal}${'.'.repeat(dots)}`;
        }
    }
    return null;
}

// How the voices below the cantus firmus rest, measure by measure, in a score to be imported
// into a notation program. A measure rest does it, so the rhythm of the chorale does not show
// through the rests, except where the cantus firmus has a fermata: the rests are cut there, so
// that every voice has the fermata at the same place. Resolves with a Map from the index of a
// line to the rest that begins on it, for each measure that can be laid out like that.
function planRests(lines) {
    const plans = new Map();
    let meterQuarters = 4;
    let measure = [];

    const finishMeasure = () => {
        const lineIndices = measure.map(({ index }) => index);
        const events = [];
        let onset = 0;
        for (const { index, token } of measure) {
            if (token === '.') continue;
            const quarters = tokenQuarters(token);
            events.push({ index, onset, quarters, fermata: token.includes(';') });
            onset += quarters;
        }
        measure = [];
        if (!events.length) return;

        const total = onset;
        // The measure in spans of rests: one for each fermata, and one for what lies between them.
        const spans = [];
        let run = null;
        for (const event of events) {
            if (event.fermata) {
                if (run) spans.push({ index: run.index, quarters: event.onset - run.onset, fermata: false });
                spans.push({ index: event.index, quarters: event.quarters, fermata: true });
                run = null;
            } else if (!run) {
                run = event;
            }
        }
        if (run) spans.push({ index: run.index, quarters: total - run.onset, fermata: false });

        const isMeasureRest = spans.length === 1 && !spans[0].fermata && Math.abs(total - meterQuarters) < 1e-9;
        const plan = new Map();
        for (const span of spans) {
            const token = quartersToToken(span.quarters);
            if (!token) return;
            plan.set(span.index, `${token}${isMeasureRest ? 'rr' : 'r'}${span.fermata ? ';' : ''}`);
        }
        for (const index of lineIndices) plans.set(index, plan);
    };

    lines.forEach((line, index) => {
        const meter = line.match(/^\*M(\d+)\/(\d+)/);
        if (meter) meterQuarters = 4 * Number(meter[1]) / Number(meter[2]);
        const tokens = line.split('\t');
        if (line.startsWith('=')) finishMeasure();
        else if (isDataLine(line)) measure.push({ index, token: tokens.at(-1) });
    });
    finishMeasure();

    return plans;
}

// Turns every voice but the last (the cantus firmus) into rests, for a file to be imported into a
// notation program. They are measure rests, see planRests.
export function restOtherVoices(kern) {
    const lines = kern.split('\n');
    const plans = planRests(lines);

    return lines.map((line, lineIndex) => {
        if (!isDataLine(line)) return line;

        const tokens = line.split('\t');
        const lastIndex = tokens.length - 1;
        const plan = plans.get(lineIndex);

        return tokens.map((token, index) => {
            if (index === lastIndex) return token;
            if (plan) return plan.get(lineIndex) ?? '.';
            const note = token.match(NOTE_TOKEN);
            return note ? `${note[1]}r${token.includes(';') ? ';' : ''}` : token;
        }).join('\t');
    }).join('\n');
}

// The cantus firmus and the bass: the first and the last spine.
const OUTER_VOICES = 'extract -k 1,$';

// The stems of the cantus firmus point up, which is what it needs as the upper voice of a piano
// system. Not those of its rests, which have none.
const STEMS_UP = 'shed -s $ -e "s#$#/#D" -E r';

// What a PDF shows of the voices that are not written out: nothing. Their notes are removed and a
// hidden rest as long as the measure stays, so that they do not widen the measures of the cantus
// firmus. restfill fills the measures that have no data in a spine. The spines are **temp until
// then, because the rhythm analysis does not take a spine without any durations; restfill
// turns them back into **kern.
function hideVoices(spines) {
    return [
        `shed -s ${spines} -e "s/kern/temp/X"`,
        `shed -s ${spines} -e "s/.*/./D"`,
        'restfill -i temp -y',
    ];
}

// The Humdrum filters (of humlib, Verovio runs them on the **kern text) that turn the chorale
// into what is downloaded.
export function choraleFilters({ format, layout }) {
    // A PDF shows the cantus firmus in a piano layout with the one voice below it, its other voices
    // are invisible anyway.
    const outerVoicesOnly = format === 'pdf' && layout === 'piano';
    const filters = [];

    if (outerVoicesOnly) filters.push(OUTER_VOICES);
    // The voices of a MEI that are not written out are rests, see restOtherVoices.
    if (format === 'pdf') filters.push(...hideVoices(outerVoicesOnly ? '1' : '1-3'));
    if (layout === 'piano') filters.push(STEMS_UP);
    // The four voices share the two staves of a piano system.
    if (layout === 'piano' && !outerVoicesOnly) filters.push('satb2gs');

    return filters;
}

// Loads the chorale into Verovio, through the Humdrum filters.
export async function createChoraleToolkit(kern, { filters = [], options = {} } = {}) {
    const toolkit = new VerovioToolkit(await createVerovioModule());
    toolkit.setOptions({
        header: 'none',
        footer: 'none',
        ...options,
    });
    toolkit.loadData(`${kern}${filters.map((filter) => `\n!!!filter: ${filter}`).join('')}`);
    return toolkit;
}

// "Bach, Johann Sebastian" as it reads in the score.
function readableName(name) {
    const [family, ...given] = name.split(',').map((part) => part.trim());
    return given.length ? `${given.join(' ')} ${family}` : family;
}

// Verovio writes one fermata for the voices that share a staff (in a piano system), and leaves out
// the fermata of the other voice. For every fermata of the **kern file that has none in the MEI
// the fermata is added, in the way Verovio writes the others: above for the first, below for the
// second voice of a staff.
function addVoiceFermatas(mei, humdrum) {
    humdrum.split('\n').forEach((line, lineIndex) => {
        if (!isDataLine(line)) return;
        line.split('\t').forEach((token, fieldIndex) => {
            if (!token.includes(';')) return;

            const position = `L${lineIndex + 1}F${fieldIndex + 1}`;
            const id = mei.match(new RegExp(`xml:id="((?:note|rest|chord|mrest)-${position})"`))?.[1];
            if (!id || mei.includes(`startid="#${id}"`)) return;

            // Next to the fermata of the same line, which stands in the measure after the staves.
            const neighbours = [...mei.matchAll(new RegExp(`<fermata\\b[^>]*startid="#[a-z]+-L${lineIndex + 1}F\\d+"[^>]*/>`, 'g'))];
            const neighbour = neighbours.at(-1);
            if (!neighbour) return;

            const layer = [...mei.slice(0, mei.indexOf(`xml:id="${id}"`)).matchAll(/<layer\b[^>]*\bn="(\d+)"/g)].at(-1)?.[1];
            const fermata = `<fermata xml:id="fermata-${position}" startid="#${id}" place="${layer === '1' ? 'above' : 'below'}" />`;
            const end = neighbour.index + neighbour[0].length;
            mei = `${mei.slice(0, end)}\n${fermata}${mei.slice(end)}`;
        });
    });
    return mei;
}

// MEI as MuseScore reads it: MEI Basic 5.1. Verovio writes its own development version (and has
// no option to name another one), so the version is named here; the notation itself is the same.
// MuseScore does not take what Verovio writes in the places it looks at, so
// - the brace or bracket of a staff group moves from the attribute staffGrp@symbol to the
//   <grpSym> element of MEI 5, and into a staff group of its own: MuseScore reads the brackets
//   of the staff groups inside the outermost one, and not the one of the outermost itself,
// - the composer is added to the title statement, where Verovio only has it in the sources,
// - the page header (title and composer as <rend> text) is left out, MuseScore cannot convert it.
export function exportChoraleMei(toolkit) {
    let mei = toolkit.getMEI({ basic: true })
        .replace(/<pgHead\b[\s\S]*?<\/pgHead>\s*/g, '')
        .replace('meiversion="6.0-dev+basic"', 'meiversion="5.1+basic"')
        .replaceAll('/schema/dev/mei-basic.rng', '/schema/5.1/mei-basic.rng');

    mei = mei.replace(/<staffGrp\b([^>]*?)\ssymbol="(\w+)"([^>]*)>([\s\S]*?)<\/staffGrp>/g, (match, before, symbol, after, content) => {
        const ids = [...content.matchAll(/<staffDef\b[^>]*?\sxml:id="([^"]+)"/g)].map(([, id]) => id);
        const span = ids.length ? ` startid="#${ids[0]}" endid="#${ids.at(-1)}"` : '';
        return `<staffGrp><staffGrp${before}${after}><grpSym symbol="${symbol}"${span}/>${content}</staffGrp></staffGrp>`;
    });

    const composer = mei.match(/<composer>\s*<persName>([^<]+)<\/persName>/)?.[1];
    if (composer) {
        mei = mei.replace(/(<titleStmt>[\s\S]*?<\/title>)/, `$1<respStmt><persName role="composer">${readableName(composer)}</persName></respStmt>`);
    }
    return addVoiceFermatas(mei, toolkit.getHumdrum());
}
