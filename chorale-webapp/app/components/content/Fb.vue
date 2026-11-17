<script setup>
const props = defineProps({
    // The figures as **fb writes them ("#6 4 3", "6/", "-7"), separated by spaces, or an array.
    v: { type: [String, Number, Array], required: true },
    // Keeps the figures in the order of v, the upper ones first, instead of sorting them with the highest at the top.
    literal: Boolean,
});

// The glyphs of SMuFL that Verovio draws a figured bass with, and the text they are copied as.
// The digits 0 to 9; the codes between them are their slashed and raised forms.
const DIGITS = ['\uEA50', '\uEA51', '\uEA52', '\uEA54', '\uEA55', '\uEA57', '\uEA5B', '\uEA5D', '\uEA60', '\uEA61'];
const SLASHED_DIGITS = { 2: '\uEA53', 4: '\uEA56', 5: '\uEA58', 6: '\uEA5C', 7: '\uEA5E', 9: '\uEA62' };
const ACCIDENTALS = { '#': ['\uEA66', '♯'], '♯': ['\uEA66', '♯'], '-': ['\uEA64', '♭'], 'b': ['\uEA64', '♭'], '♭': ['\uEA64', '♭'], 'n': ['\uEA65', '♮'], '♮': ['\uEA65', '♮'] };

// An accidental without a number stands for the third.
const numberOf = (figure) => Number(figure.match(/\d+/)?.[0] ?? 3);

const figures = computed(() => {
    const values = (Array.isArray(props.v) ? props.v : String(props.v).split(/\s+/)).map(String).filter(Boolean);
    // From the bottom up, the order in which they are copied.
    const fromBottom = props.literal ? values.toReversed() : values.toSorted((a, b) => numberOf(a) - numberOf(b));
    return fromBottom.map((figure) => {
        const parts = [...figure.matchAll(/(\d)\/|./g)].map(([part, slashedDigit]) => {
            if (slashedDigit) return [SLASHED_DIGITS[slashedDigit] ?? DIGITS[slashedDigit], part];
            if (/\d/.test(part)) return [DIGITS[part], part];
            return ACCIDENTALS[part] ?? [part, part];
        });
        return { glyphs: parts.map(([glyph]) => glyph).join(''), text: parts.map(([, text]) => text).join('') };
    });
});
</script>

<template>
    <span class="figured-bass" :class="figures.length > 1 && ['stacked', figures.length === 2 && 'pair']">
        <template v-for="(figure, index) in figures" :key="index">
            <template v-if="index">{{ ' ' }}</template>
            <span class="figure" :data-glyphs="figure.glyphs">{{ figure.text }}</span>
        </template>
    </span>
</template>

<style scoped>
/* The glyphs of the music font are smaller than the digits of the text. */
.figured-bass {
    display: inline-block;
    margin-inline: 0.06em;
    font-size: 1.45em;
    line-height: 1;
    white-space: nowrap;
    /* The heavy glyphs seem to sit low on the baseline. */
    vertical-align: 0.04em;
}

/* Stacked by the width of the widest figure, so that the copied text stays on one line, and turned
   upside down with each figure turned back, so that the lowest figure, the first in the text, is at the bottom. */
.stacked {
    width: min-content;
    scale: 1 -1;
    white-space: normal;
    text-align: center;
    vertical-align: middle;
    margin-inline: 0.25em;
    font-size: 0.9em;
    line-height: 0.8;
}

.figure {
    position: relative;
    -webkit-text-fill-color: transparent;
}

/* Two figures fit into a line of text at a larger size. */
.pair {
    font-size: 1.05em;
}

/* As high as a figure, whatever font a sign of the copied text falls back to. */
.stacked .figure {
    display: inline-block;
    scale: 1 -1;
    height: 0.8em;
    vertical-align: top;
}

/* The glyphs of the music font over the transparent text, which is what is copied. */
.figure::before {
    content: attr(data-glyphs);
    position: absolute;
    left: 50%;
    translate: -50% 0;
    font-family: 'Bravura Figured Bass';
    -webkit-text-fill-color: currentColor;
}
</style>
