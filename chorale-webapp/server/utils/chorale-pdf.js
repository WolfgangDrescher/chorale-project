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

// PDF only knows its own base fonts, so the text of the score is set in Times.
function fontCallback(family, bold, italic) {
    if (bold && italic) return 'Times-BoldItalic';
    if (bold) return 'Times-Bold';
    if (italic) return 'Times-Italic';
    return 'Times-Roman';
}

// svg-to-pdfkit clips to the view box of the inner <svg> unless that says overflow="visible", which
// is what lets the brace and the measure numbers stand outside of the staves, in the margin of the page.
function prepareSvg(svg) {
    return svg.replace('<svg class="definition-scale"', '<svg class="definition-scale" overflow="visible"');
}

// Typesets the score on A4 pages and resolves with the PDF as a Buffer.
export async function renderChoralePdf(kern, { filters }) {
    const pageWidth = A4_SHORT_SIDE;
    const pageHeight = A4_LONG_SIDE;

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
            // the measure numbers stand outside, see prepareSvg).
            pageMarginTop: 0,
            pageMarginLeft: 0,
            pageMarginRight: 0,
            pageMarginBottom: 0,
            spacingSystem: SPACING_SYSTEM,
            spacingLinear: SPACING_LINEAR,
            justifyVertically: true,
        },
    });

    const doc = new PDFDocument({
        size: [verovioUnitToPoints(pageWidth), verovioUnitToPoints(pageHeight)],
        margin: 0,
        compress: true,
        autoFirstPage: false,
    });
    const chunks = [];
    doc.on('data', (chunk) => chunks.push(chunk));
    const finished = new Promise((resolve, reject) => {
        doc.on('end', () => resolve(Buffer.concat(chunks)));
        doc.on('error', reject);
    });

    const margin = verovioUnitToPoints(PAGE_MARGIN);

    for (let page = 1; page <= toolkit.getPageCount(); page++) {
        doc.addPage();
        SVGtoPDF(doc, prepareSvg(toolkit.renderToSVG(page, {})), margin, margin, { fontCallback });
    }
    doc.end();
    toolkit.destroy();

    return finished;
}
