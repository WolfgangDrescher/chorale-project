<script setup>
const props = defineProps({
    // Degrees as **deg writes them ("4+", "6-") or with accidentals ("#4", "b6", "♯4", "♭6"): one, several
    // separated by spaces, or an array.
    v: { type: [String, Number, Array], required: true },
    // A degree is shown on a shaded disc, circled in the bass, with a caret in an upper voice.
    bass: Boolean,
    upper: Boolean,
    // Draws the accidentals as arrows too.
    arrow: Boolean,
});

// + and - are drawn as arrows, the accidentals as they are.
const ALTERATIONS = { '+': '↑', '-': '↓', '#': '♯', '♯': '♯', 'b': '♭', '♭': '♭' };
const AS_ARROWS = { '♯': '↑', '♭': '↓' };

const degrees = computed(() => {
    const values = Array.isArray(props.v) ? props.v : String(props.v).split(/\s+/);
    return values.map(String).filter(Boolean).map((value) => {
        const [, before, number, after] = value.match(/^([#b♯♭]*)(\d+)([+\-#b♯♭]*)$/) ?? [null, '', value, ''];
        return { number, alterations: [...before, ...after].map((sign) => {
            const alteration = ALTERATIONS[sign];
            return props.arrow ? (AS_ARROWS[alteration] ?? alteration) : alteration;
        }) };
    });
});
</script>

<template>
    <span class="whitespace-nowrap font-semibold lining-nums tabular-nums">
        <template v-for="(degree, index) in degrees" :key="index">
            <span v-if="index" class="px-0.5">–</span>
            <span
                :class="[
                    (bass || upper) && 'inline-block h-[1.5em] min-w-[1.5em] rounded-full bg-neutral-500/10 px-[0.2em] text-center align-middle text-[0.85em] leading-[1.5em]',
                    bass && 'ring-1 ring-current ring-inset',
                ]"
            >
                <span :class="upper && 'caret'">
                    <span v-for="(alteration, alterationIndex) in degree.alterations" :key="alterationIndex" :class="{ '↑': 'arrow', '↓': 'arrow down' }[alteration] ?? 'text-[0.9em]'">{{ alteration }}</span>{{ degree.number }}
                </span>
            </span>
        </template>
    </span>
</template>

<style scoped>
/* Inline boxes only: in copied text, a flex item or an absolute element would break the line. */
.caret {
    position: relative;
    display: inline-block;
    line-height: 1;
    padding-top: 0.32em;
    vertical-align: 0.1em;
    top: -0.07em;
}

.caret::before {
    content: '';
    position: absolute;
    top: 0.07em;
    left: 50%;
    width: 0.64em;
    height: 0.29em;
    translate: -50% 0;
    background: currentColor;
    mask: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 12 5'%3E%3Cpath d='M.3 4.7Q3.5 2.1 5.5.25h1q2 1.85 5.2 4.45Q8.5 3.5 6 2.45 3.5 3.5.3 4.7z'/%3E%3C/svg%3E") no-repeat center / contain;
}

/* The arrow of the text font is long and thin: the character stays for copying, a drawn one is shown. */
.arrow {
    display: inline-block;
    -webkit-text-fill-color: transparent;
    background: currentColor;
    mask: url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 6 12'%3E%3Cpath d='M3 11.2V4' stroke='black' stroke-width='1.3' stroke-linecap='round'/%3E%3Cpath d='M.7 4.9 3 .7l2.3 4.2z' stroke='black' stroke-width='.6' stroke-linejoin='round'/%3E%3C/svg%3E") no-repeat center / contain;
    width: 0.38em;
    height: 0.72em;
    line-height: 0.72em;
    margin-right: 0.04em;
}

.arrow.down {
    scale: 1 -1;
}
</style>
