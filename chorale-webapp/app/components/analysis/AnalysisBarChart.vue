<script setup>
import * as Plot from '@observablehq/plot';
import { useDebounceFn, useResizeObserver } from '@vueuse/core';

const props = defineProps({
    result: { type: Object, required: true },
    // The result, or one of the variants it states (see analyses/README.md).
    analysis: { type: Object, required: true },
    // How many groups of the first level get a bar: as many as the table shows before it opens.
    top: { type: Number, default: 10 },
});

const emit = defineEmits(['more']);

const { t } = useI18n();
const format = inject('analysisFormat');

// A part under the pointer keeps its step of the color, mixed towards red, so the order of the
// weights stays readable.
const color = (palette, step) => `var(--ui-color-${palette}-${step})`;
const active = (palette, step) => `color-mix(in oklab, ${color('error', step)} 75%, ${color(palette, step)})`;
const BAR = color('primary', 600);
const BAR_ACTIVE = active('primary', 600);
const REST = color('neutral', 300);
const REST_ACTIVE = active('neutral', 300);
// The right margin, which holds the number and the tip at the end of the longest bar.
const TIP_WIDTH = 300;
const INK = '#0b0b0b';
const SURFACE = '#ffffff';

const bars = computed(() => props.analysis.groups.slice(0, props.top).map((group) => ({
    label: format.label(group.label),
    matches: group.matchCount,
    share: group.matchCount / props.analysis.matchCount,
    choraleCount: group.choraleCount,
    query: group.query,
    groups: group.groups ?? [],
})));

const rest = computed(() => {
    const groups = props.analysis.groups.slice(props.top);
    const matches = groups.reduce((sum, group) => sum + group.matchCount, 0) + props.analysis.omittedMatchCount;
    if (!matches) return null;
    return {
        rest: true,
        label: t('analysisOther'),
        matches,
        share: matches / props.analysis.matchCount,
        rows: groups.length + (props.analysis.omittedMatchCount ? 1 : 0),
    };
});
const rows = computed(() => (rest.value ? [...bars.value, rest.value] : bars.value));

// The metric weights, strongest first, as steps of the primary color from dark to light: an
// ordinal ramp, whose order reads in the lightness and which keeps a bar one bar.
const WEIGHTS = [
    { value: 's', step: 700 },
    { value: 'hs', step: 600 },
    { value: 'w', step: 500 },
    { value: 'u', step: 400 },
].map((weight) => ({ ...weight, color: color('primary', weight.step), active: active('primary', weight.step) }));
const byWeight = computed(() => {
    const reads = props.result.groupBy[1]?.reads;
    return reads?.length === 1 && reads[0].feature === 'metweight';
});

const segments = computed(() => (byWeight.value ? bars.value.flatMap((bar) => {
    const parts = WEIGHTS
        .map((weight) => ({ weight, group: bar.groups.find((group) => group.values?.[0]?.[0] === weight.value) }))
        .filter((part) => part.group);
    let start = 0;
    return parts.map(({ weight, group }, index) => {
        const segment = {
            label: bar.label,
            weight: weight.value,
            color: weight.color,
            active: weight.active,
            x1: start,
            x2: start + group.matchCount,
            total: bar.matches,
            matches: group.matchCount,
            share: group.matchCount / bar.matches,
            query: group.query,
            last: index === parts.length - 1,
        };
        start = segment.x2;
        return segment;
    });
}) : []));

function segmentTipText(segment) {
    return `${t(`metweightShort.${segment.weight}`)}: ${format.count(segment.matches)} (${format.percent(segment.share)})`;
}

function barText(bar) {
    return `${format.count(bar.matches)} · ${format.percent(bar.share)}`;
}

function tipText(bar) {
    return bar.rest ? t('analysisMoreRows', bar.rows) : t('choraleCount', bar.choraleCount);
}

const TIP = {
    dx: 112,
    anchor: 'left',
    lineWidth: 40,
    fill: SURFACE,
    stroke: SURFACE,
    textPadding: 6,
    pathFilter: 'drop-shadow(0 2px 6px rgb(0 0 0 / 0.18))',
};

function barMarks(data) {
    return [
        Plot.barX(data, { x: 'matches', y: 'label', fill: (bar) => (bar.rest ? REST : BAR), insetTop: 3, insetBottom: 3, rx2: 4 }),
        Plot.barX(data, Plot.pointerY({ x: 'matches', y: 'label', fill: (bar) => (bar.rest ? REST_ACTIVE : BAR_ACTIVE), insetTop: 3, insetBottom: 3, rx2: 4 })),
        Plot.tip(data, Plot.pointerY({ x: 'matches', y: 'label', title: tipText, ...TIP })),
    ];
}

let drawn = [];

function weightMarks() {
    const parts = segments.value;
    const bar = { x1: 'x1', x2: 'x2', y: 'label', fill: 'color', insetTop: 3, insetBottom: 3 };
    // The pointer finds the row first and then the closest point along it (pointerY). Points along the
    // whole width of each part make that the part under the pointer, where the middle of a wide
    // part could be farther than that of its narrow neighbour.
    const longest = Math.max(...parts.map((part) => part.total));
    const step = longest / 300;
    const points = parts.flatMap((part) => {
        const count = Math.max(1, Math.round((part.x2 - part.x1) / step));
        return Array.from({ length: count }, (_, index) => ({ ...part, px: part.x1 + ((index + 0.5) * (part.x2 - part.x1)) / count }));
    });
    // Right of its end the pointer is about the whole bar: its chorales in the tip, and its query, of
    // all the weights, for the click.
    // The gray bar of the rest is about all of it, along it and beside it.
    const wholeBars = rows.value.flatMap((whole) => Array.from({ length: whole.rest ? 120 : 60 }, (_, index) => ({
        ...whole,
        whole: true,
        x1: 0,
        x2: whole.matches,
        total: whole.matches,
        px: whole.rest ? ((index + 0.5) * (whole.matches + longest)) / 120 : whole.matches + ((index + 0.5) * longest) / 60,
    })));
    const targets = [...points, ...wholeBars];
    const pointer = { px: 'px', py: 'label' };
    const title = (point) => (point.whole ? tipText(point) : segmentTipText(point));
    const inner = parts.filter((part) => !part.last);
    const ends = parts.filter((part) => part.last);
    const restParts = rest.value ? [{ ...rest.value, x1: 0, x2: rest.value.matches, color: REST, active: REST_ACTIVE }] : [];
    drawn = [...inner, ...ends, ...restParts].map((part) => ({ part }));
    return [
        Plot.barX(inner, { ...bar, insetRight: 1, className: 'analysis-part' }),
        Plot.barX(ends, { ...bar, rx2: 4, className: 'analysis-part' }),
        Plot.barX(restParts, { ...bar, rx2: 4, className: 'analysis-part' }),
        Plot.tip(targets, Plot.pointerY({ ...pointer, x: 'total', y: 'label', title, ...TIP })),
    ];
}

// The part under the pointer, or all the parts of the bar, in their colors mixed towards red. The
// gaps between them stay as they are, which a mark over a whole bar would fill.
function highlight(point) {
    for (const { part, rect } of drawn) {
        const hit = point && part.label === point.label && (point.whole || part.weight === point.weight);
        rect?.setAttribute('fill', hit ? part.active : part.color);
    }
}

const container = useTemplateRef('container');
const drawerOpen = ref(false);
const activeQuery = ref('');
let plot = null;

function render() {
    if (!container.value || !rows.value.length) return;
    const data = rows.value;

    plot?.remove();
    plot = Plot.plot({
        width: container.value.clientWidth,
        height: data.length * 36 + 44,
        marginLeft: 180,
        marginRight: TIP_WIDTH,
        style: { fontFamily: 'inherit', fontSize: '17px', color: INK, background: SURFACE, overflow: 'visible' },
        // No label: the numbers at the ends of the bars and the table below say what they count.
        // Up to the longest bar, whatever lies beyond it to point at (see weightMarks).
        x: { domain: [0, Math.max(...data.map((bar) => bar.matches))], nice: true, label: null, grid: true, tickFormat: format.count },
        y: { domain: data.map((bar) => bar.label), label: null, tickSize: 0 },
        marks: [
            ...(byWeight.value ? weightMarks() : barMarks(data)),
            Plot.text(data, { x: 'matches', y: 'label', text: barText, dx: 6, textAnchor: 'start', fill: INK }),
        ],
    });
    // A part with a round end is a path.
    const rects = plot.querySelectorAll('.analysis-part rect, .analysis-part path');
    drawn = drawn.map(({ part }, index) => ({ part, rect: rects[index] }));
    plot.addEventListener('input', () => {
        if (byWeight.value) highlight(plot.value);
        plot.style.cursor = plot.value?.query || plot.value?.rest ? 'pointer' : 'default';
    });
    container.value.append(plot);
}

// A click runs the query of the bar under the pointer, or shows all the rows of the table for the
// gray one. It is caught before it reaches the plot,
// which would otherwise pin the tip to that bar and take the next click to let go of it.
function onPointerDown(event) {
    event.stopPropagation();
    if (plot?.value?.rest) emit('more');
    if (!plot?.value?.query) return;
    activeQuery.value = JSON.stringify(plot.value.query, null, 4);
    drawerOpen.value = true;
}

onMounted(render);
useResizeObserver(container, useDebounceFn(render, 150));
onBeforeUnmount(() => plot?.remove());
</script>

<template>
    <div class="my-6">
        <div ref="container" class="analysis-bar-chart w-full" @pointerdown.capture="onPointerDown" />
        <ul v-if="byWeight" class="mt-2 flex flex-wrap gap-x-5 gap-y-1 text-xs text-muted" :style="{ paddingLeft: '180px' }">
            <li v-for="weight in WEIGHTS" :key="weight.value" class="flex items-center gap-1.5">
                <span class="inline-block size-3 rounded-sm" :style="{ backgroundColor: weight.color }" />
                {{ t(`metweight.${weight.value}`) }}
            </li>
        </ul>
        <QueryDrawer v-model:open="drawerOpen" :query="activeQuery" sort-by="density" />
    </div>
</template>

<style scoped>
/* The frame of the tip, drawn wide in the color of its fill, has round corners this way. */
.analysis-bar-chart :deep(g[aria-label="tip"] path) {
    stroke-width: 8px;
    stroke-linejoin: round;
}
</style>
