import PDFDocument from 'pdfkit';
import SVGtoPDF from 'svg-to-pdfkit';

// Verovio lays out in tenths of a millimetre (mmOutput), PDF in points.
function verovioUnitToPoints(unit) {
    return unit / 10 * 72 / 25.4;
}

// A4 in tenths of a millimetre.
const A4_SHORT_SIDE = 2100;
const A4_LONG_SIDE = 2970;
const PAGE_MARGIN = 150;

// Large enough to write the notes in with a pen. Without the other voices Verovio finds little
// to space the measures by, so they get more room (spacingLinear) than its default of 0.25.
// The average chorale as a piano setting still fits on one page, so the staves are not set too
// large and the systems are not far apart.
const SCORE_SCALE = 85;
const SPACING_LINEAR = 0.35;
const SPACING_SYSTEM = 10;

// The sizes of the text on the page, in points: the title is larger than the subtitle (and the
// composer, who stands on its line), which is larger than the footer.
const TITLE_SIZE = 20;
const SUBTITLE_SIZE = 12;
const FOOTER_SIZE = 10;

// The font of the website, from the @fontsource package. Its files are server assets, see nuxt.config.ts.
const FONTS = {
    regular: 'alegreya-sans-latin-400-normal.woff',
    italic: 'alegreya-sans-latin-400-italic.woff',
    bold: 'alegreya-sans-latin-700-normal.woff',
};

// What the PDF is set in without the font: its own base fonts.
const FALLBACK_FONTS = { regular: 'Helvetica', italic: 'Helvetica-Oblique', bold: 'Helvetica-Bold' };

// Registers the font with the document and resolves with the names of its styles. Where the
// package is not installed (npm install), the base fonts of PDF stand in for it.
async function registerFonts(doc) {
    const assets = useStorage('assets:fonts');
    const fonts = {};
    for (const [style, file] of Object.entries(FONTS)) {
        const data = await assets.getItemRaw(file);
        if (!data) {
            console.warn(`The font file ${file} was not found, the PDF is set in Helvetica. Run "npm install" (it is part of @fontsource/alegreya-sans) and restart the server.`);
            return FALLBACK_FONTS;
        }
        fonts[style] = `AlegreyaSans-${style}`;
        doc.registerFont(fonts[style], Buffer.from(data));
    }
    return fonts;
}

// The text in the score (the measure numbers) is set in the same font.
function createFontCallback(fonts) {
    return (family, bold, italic) => (bold ? fonts.bold : italic ? fonts.italic : fonts.regular);
}

// svg-to-pdfkit clips to the view box of the inner <svg> unless that says overflow="visible", which
// is what lets the brace and the measure numbers stand outside of the staves, in the margin of the page.
function prepareSvg(svg) {
    return svg.replace('<svg class="definition-scale"', '<svg class="definition-scale" overflow="visible"');
}

// Where Verovio writes the number of a measure, as the size of its text and the height of its
// baseline above the top line of the staff, in the units of the SVG.
const MEASURE_NUMBER_SIZE = 324;
const MEASURE_NUMBER_RISE = 243;

// The numbers of the measures by their ids, in the order of the score. A pickup measure has none.
function readMeasureNumbers(mei) {
    return new Map([...mei.matchAll(/<measure\b([^>]*)>/g)].map(([, attributes]) => [
        attributes.match(/xml:id="([^"]+)"/)?.[1],
        attributes.match(/\bn="([^"]*)"/)?.[1],
    ]));
}

// Verovio writes the number of the measure that starts a system, but not where that is a pickup
// measure (the one after a repeat, which completes the measure before it), because it has none.
// The first full measure of such a system gets its number instead, above the barline in front of
// it. Resolves with those numbers as { text, x, y, size } in points, from the corner of the SVG.
function pickupSystemNumbers(svg, measureNumbers) {
    const unit = parseFloat(svg.match(/<svg width="([\d.]+)mm"/)[1]) * 72 / 25.4 / Number(svg.match(/viewBox="0 0 (\d+)/)[1]);
    // The page margins shift the content of the page, and with it the coordinates of the staves.
    const [, shiftX, shiftY] = svg.match(/class="page-margin" transform="translate\((-?[\d.]+), (-?[\d.]+)\)"/) ?? [, 0, 0];
    const [firstMeasure] = measureNumbers.keys();
    const numbers = [];

    for (const system of svg.split('class="system"').slice(1)) {
        const measures = [...system.matchAll(/<g id="([^"]+)" class="measure">/g)];
        // The first measure of the score is a pickup measure as well, and needs no number.
        if (!measures.length || measureNumbers.get(measures[0][1]) || measures[0][1] === firstMeasure) continue;

        const full = measures.find(([, id]) => measureNumbers.get(id));
        const topLine = full && system.slice(full.index).match(/class="staff">\s*<path d="M(-?\d+) (-?\d+)/);
        if (!topLine) continue;

        numbers.push({
            text: measureNumbers.get(full[1]),
            x: (Number(topLine[1]) + Number(shiftX)) * unit,
            y: (Number(topLine[2]) + Number(shiftY) - MEASURE_NUMBER_RISE) * unit,
            size: MEASURE_NUMBER_SIZE * unit,
        });
    }
    return numbers;
}

// Typesets the score on A4 pages and resolves with the PDF as a Buffer. The title, the subtitle
// and the footer are written by PDFKit, `t` translates what is in the footer.
export async function renderChoralePdf(kern, { filters, chorale, t, siteUrl }) {
    const pageWidth = A4_SHORT_SIDE;
    const pageHeight = A4_LONG_SIDE;
    const title = [chorale.number, chorale.title].filter(Boolean).join('. ');

    const toolkit = await createChoraleToolkit(kern, {
        filters,
        options: {
            adjustPageHeight: false,
            breaks: 'auto',
            mmOutput: true,
            scale: SCORE_SCALE,
            pageWidth: (pageWidth - PAGE_MARGIN * 2) * (100 / SCORE_SCALE),
            pageHeight: (pageHeight - PAGE_MARGIN * 2) * (100 / SCORE_SCALE),
            // The staves run from the left to the right edge of the text of the page (the brace and
            // the measure numbers stand outside, see prepareSvg). Above them is room for the title
            // and the subtitle, below for the footer.
            pageMarginTop: 180,
            pageMarginLeft: 0,
            pageMarginRight: 0,
            pageMarginBottom: 60,
            spacingSystem: SPACING_SYSTEM,
            spacingLinear: SPACING_LINEAR,
            justifyVertically: true,
        },
    });

    const measureNumbers = readMeasureNumbers(toolkit.getMEI());

    const doc = new PDFDocument({
        size: [verovioUnitToPoints(pageWidth), verovioUnitToPoints(pageHeight)],
        margin: 0,
        compress: true,
        autoFirstPage: false,
        info: { Title: title },
    });
    const fonts = await registerFonts(doc);
    const chunks = [];
    doc.on('data', (chunk) => chunks.push(chunk));
    const finished = new Promise((resolve, reject) => {
        doc.on('end', () => resolve(Buffer.concat(chunks)));
        doc.on('error', reject);
    });

    const margin = verovioUnitToPoints(PAGE_MARGIN);
    const contentWidth = verovioUnitToPoints(pageWidth - PAGE_MARGIN * 2);
    const footerTop = verovioUnitToPoints(pageHeight) - 40;
    // Two lines in a corner of the footer, the second of them a link where there is an address.
    const footer = (align, [first, second], link) => {
        doc.text(first, margin, footerTop, { width: contentWidth, align });
        doc.text(second, margin, doc.y, { width: contentWidth, align, link });
    };

    for (let page = 1; page <= toolkit.getPageCount(); page++) {
        doc.addPage();
        const svg = toolkit.renderToSVG(page, {});
        SVGtoPDF(doc, prepareSvg(svg), margin, margin, { fontCallback: createFontCallback(fonts) });
        for (const { text, x, y, size } of pickupSystemNumbers(svg, measureNumbers)) {
            doc.font(fonts.italic).fontSize(size).fillColor('black');
            doc.text(text, margin + x - 20, margin + y, { width: 40, align: 'center', baseline: 'alphabetic' });
        }

        doc.font(fonts.regular).fontSize(FOOTER_SIZE).fillColor('black');
        footer('left', ['Chorale Project', siteUrl], siteUrl);
        footer('center', [title, t('pageNumber', { page })]);
        footer('right', ['Prof. Wolfgang Drescher', 'w.drescher@mh-freiburg.de'], 'mailto:w.drescher@mh-freiburg.de');

        if (page === 1) {
            doc.fontSize(TITLE_SIZE).text(title, margin, margin - 5, { width: contentWidth });
            // The subtitle and the composer share a baseline, below the title.
            doc.fontSize(SUBTITLE_SIZE);
            const subtitleTop = doc.y + 2;
            if (chorale.bwv) doc.text(chorale.bwv, margin, subtitleTop, { width: contentWidth, align: 'left' });
            if (chorale.composer) doc.text(readableName(chorale.composer), margin, subtitleTop, { width: contentWidth, align: 'right' });
        }
    }
    doc.end();
    toolkit.destroy();

    return finished;
}
