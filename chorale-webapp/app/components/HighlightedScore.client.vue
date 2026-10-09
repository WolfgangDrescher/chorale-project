<script setup lang="ts">
const props = defineProps<{
    // Where the score comes from: a piece served by the app (pieceId), or -- for a score that
    // only exists as text, like an upload -- the **kern data itself. scoreData wins when both
    // are given.
    pieceId?: String,
    scoreData?: String,
    verovioOptions?: {
        type: Object,
        default: () => ({}),
    },
    notes?: NotesProp,
    lines?: LinesProp,
    sections?: SectionsProp,
    connections?: ConnectionsProp,
    filters?: Array<string>,
    horizontal?: Boolean,
    scrollToFirstSection?: Boolean,
    // In the horizontal view: the line of a note to keep in view, scrolled to again whenever
    // it changes. Wins over scrollToFirstSection.
    scrollToLine?: Number,
}>();

const emit = defineEmits<{
    // A click on (or near) a note: its element id, and the line and voice read off it.
    noteClick: [note: { id: string, line: number, voice: number }],
}>();

defineOptions({ inheritAttrs: false });

const { resolvedNotes, resolvedLines, resolvedSections } = useResolveHighlightedScoreProps(props);

const verovioCanvas = ref(null);

const { loadScore, setScore, applyScoreFormatting, formattedScoreData } = useScoreFormatter();

watch(() => props.filters, (filters) => {
    applyScoreFormatting([], filters);
});

const verovioCanvasAttrs = computed(() => {
    return Object.assign({
        pageMargin: 50,
        viewMode: props.horizontal ? 'horizontal' : 'vertical',
        options: {
            ...props.verovioOptions,
            svgBoundingBoxes: true,
            // svgViewBox: true,
        },
        data: formattedScoreData.value,
    });
});

const scoreContainer = useTemplateRef('scoreContainer');
const markerContainer = useTemplateRef('markerContainer');
const wrapperElem = useTemplateRef('wrapperElem');
const scoreKey = ref(Date.now());

const markerContainerStyle = reactive<{
    width: string | undefined,
    height: string | undefined,
}>({
    width: undefined,
    height: undefined,
});

const { scrollElementIntoView } = useHorizontalScroll();

// The first background color found walking up from the element, since the page behind the score
// may not be white.
function backgroundBehind(elem) {
    for (let current = elem; current; current = current.parentElement) {
        const color = getComputedStyle(current).backgroundColor;
        if (color && color !== 'transparent' && !/rgba?\(.*,\s*0\)$/.test(color)) return color;
    }
    return '#fff';
}

// In the horizontal view the system start (labels, brace, clef, key and time signature) stays at
// the left edge while the score scrolls: a clone of just those parts, on an opaque background so
// the notes scrolling underneath do not show through. It sits above the markers (up to z-30).
function useStickySystemStart({ scoreContainer, wrapperElem, horizontal }) {
    const PADDING = 8;
    // A fade on its right edge hints that the score continues to its left; it is only shown once
    // the score is scrolled, so it does not mark the true start of the piece.
    const FADE_WIDTH = 28;
    const FADE_DISTANCE = 40;

    const elem = useTemplateRef('stickySystemStartElem');
    const style = reactive({ width: '0px', height: '0px', backgroundColor: '#fff' });
    const scrollLeft = ref(0);
    const fadeStyle = computed(() => ({
        left: '100%',
        width: `${FADE_WIDTH}px`,
        height: style.height,
        backgroundImage: `linear-gradient(to right, ${style.backgroundColor}, transparent)`,
        opacity: Math.min(scrollLeft.value / FADE_DISTANCE, 1),
    }));

    function onScroll() {
        scrollLeft.value = wrapperElem.value?.scrollLeft ?? 0;
    }

    function update() {
        const target = elem.value;
        if (!target) return;
        target.replaceChildren();
        style.width = '0px';
        const svg = scoreContainer.value?.querySelector('svg');
        const firstMeasure = svg?.querySelector('g.measure');
        const system = firstMeasure?.parentElement;
        if (!horizontal() || !svg || !firstMeasure || !system) return;

        const svgRect = svg.getBoundingClientRect();
        const eventLefts = Array.from(firstMeasure.querySelectorAll('.note, .rest, .mRest'), el => el.getBoundingClientRect().left);
        const firstEventLeft = eventLefts.length ? Math.min(...eventLefts) : Infinity;
        let right = 0;

        // Only what the system start shows is cloned: the ancestors of the system as empty shells (they
        // carry the scale and viewBox), then the system's own line, brace and labels, and per staff
        // its lines and the leading clef, key and time signature. Glyphs are <use> references into
        // the original's <defs>, which are not cloned and resolve within the document.
        const chain = [];
        for (let el = system; el && el !== svg; el = el.parentElement) chain.unshift(el);
        const root = svg.cloneNode(false);
        let shell = root;
        for (const el of chain) shell = shell.appendChild(el.cloneNode(false));

        for (const child of system.children) {
            if (child.matches('path, .grpSym, .label, .labelAbbr')) shell.appendChild(child.cloneNode(true));
        }
        const measureClone = shell.appendChild(firstMeasure.cloneNode(false));
        for (const staff of firstMeasure.querySelectorAll(':scope > .staff')) {
            const staffClone = measureClone.appendChild(staff.cloneNode(false));
            for (const child of staff.children) {
                if (child.matches('path')) {
                    staffClone.appendChild(child.cloneNode(true));
                } else if (child.matches('.clef, .keySig, .meterSig')) {
                    const rect = child.getBoundingClientRect();
                    if (rect.right > firstEventLeft + 1) continue;
                    right = Math.max(right, rect.right - svgRect.left);
                    staffClone.appendChild(child.cloneNode(true));
                }
            }
        }
        if (!right) return;

        // Duplicate ids would only get in the way of lookups in the original. The root keeps its id:
        // verovio's <style> rules are scoped to it, so the clone is drawn by the original's styles.
        for (const el of root.querySelectorAll('[id]')) el.removeAttribute('id');
        target.appendChild(root);
        style.width = `${Math.ceil(right + PADDING)}px`;
        style.height = `${svgRect.height}px`;
        style.backgroundColor = backgroundBehind(wrapperElem.value);
    }

    function containsPoint(x, y) {
        const rect = elem.value?.getBoundingClientRect();
        return !!rect?.width && x >= rect.left && x <= rect.right && y >= rect.top && y <= rect.bottom;
    }

    return { style, fadeStyle, update, onScroll, containsPoint };
}

const {
    style: stickySystemStartStyle,
    fadeStyle: stickySystemStartFadeStyle,
    update: updateStickySystemStart,
    onScroll,
    containsPoint: stickySystemStartContainsPoint,
} = useStickySystemStart({ scoreContainer, wrapperElem, horizontal: () => props.horizontal });

// How far from a notehead, in pixels, a click still counts as a click on it.
const NOTE_CLICK_PADDING = 10;

function noteAtPoint(x: number, y: number) {
    let nearest: Element | null = null;
    let nearestDistance = Infinity;
    for (const note of scoreContainer.value?.querySelectorAll('g.note:not(.bounding-box)') ?? []) {
        const rect = (note.querySelector('.notehead') ?? note).getBoundingClientRect();
        const dx = Math.max(rect.left - x, 0, x - rect.right);
        const dy = Math.max(rect.top - y, 0, y - rect.bottom);
        if (dx > NOTE_CLICK_PADDING || dy > NOTE_CLICK_PADDING) continue;
        const distance = Math.hypot(rect.left + rect.width / 2 - x, rect.top + rect.height / 2 - y);
        if (distance < nearestDistance) {
            nearest = note;
            nearestDistance = distance;
        }
    }
    return nearest;
}

// Listens on the wrapper rather than on the notes, so a click on a highlight marker above a
// note reaches it too.
function onClick(event: MouseEvent) {
    // The system start covers the scrolled score, so a click on it is not a click on a note below.
    if (stickySystemStartContainsPoint(event.clientX, event.clientY)) return;
    const note = noteAtPoint(event.clientX, event.clientY);
    const match = note?.id.match(/^note-L(\d+)F(\d+)/);
    if (!note || !match) return;
    emit('noteClick', { id: note.id, line: Number(match[1]), voice: Number(match[2]) });
}

async function scrollToLineNumber(line: number) {
    if (!props.horizontal || !scoreContainer.value || !wrapperElem.value) return;
    await scrollElementIntoView(`g[id^="note-L${line}F"]`, scoreContainer.value, wrapperElem.value, true);
}

watch(() => props.scrollToLine, (line) => {
    if (line) scrollToLineNumber(line);
});

async function onScoreIsReady() {
    if (props.scrollToLine) return scrollToLineNumber(props.scrollToLine);
    if (!props.scrollToFirstSection) return;
    if (!resolvedSections.value?.length) return;
    if (!scoreContainer.value || !wrapperElem.value) return;

    const first = resolvedSections.value[0]?.items[0];
    if (!first) return
    const selector = `g[id^="note-L${first.startLine}"]`;

    await scrollElementIntoView(selector, scoreContainer.value, wrapperElem.value, true);
}

function updateMarkerWidth() {
    if (props.horizontal && scoreContainer.value && markerContainer.value) {
        const width = scoreContainer.value.querySelector('svg')?.getAttribute('width');
        if (width) {
            markerContainerStyle.width = width;
        }
        const height = scoreContainer.value.querySelector('svg')?.getAttribute('height');
        if (height) {
            markerContainerStyle.height = height;
        }
    }
    if (!props.horizontal) {
        markerContainerStyle.width = undefined;
        markerContainerStyle.height = undefined;
    }
}

async function mutationObserverEvent() {
    scoreKey.value = Date.now();
    await nextTick();
    updateMarkerWidth();
    updateStickySystemStart();
}

const mutationObserver = ref<MutationObserver | null>(null);

function setupMutationObserver() {
    if (mutationObserver.value) {
        mutationObserver.value.disconnect();
    }
    mutationObserver.value = new MutationObserver(mutationObserverEvent);
    if (scoreContainer.value) {
        mutationObserver.value.observe(scoreContainer.value, {
            // attributes: true,
            childList: true,
            subtree: true,
        });
    }
}

watch(() => props.horizontal, async () => {
    scoreKey.value = Date.now();
    await nextTick();
    setupMutationObserver();
    updateStickySystemStart();
});

watch(() => props.scoreData, (data) => {
    if (data) setScore(data, props.filters);
});

onMounted(async () => {
    if (props.scoreData) setScore(props.scoreData, props.filters);
    else loadScore(props.pieceId, props.filters);
    await nextTick();
    updateMarkerWidth();
    updateStickySystemStart();
    setupMutationObserver();
});
</script>

<template>
    <div class="relative" :class="horizontal && 'overflow-x-auto'" ref="wrapperElem" :key="horizontal ? 'horizontal' : 'vertical'" @click="onClick" @scroll.passive="onScroll">
        <div class="relative" :class="horizontal && 'w-max min-w-full'">
            <div v-if="horizontal" class="sticky left-0 z-40 h-0" :style="{ width: stickySystemStartStyle.width }">
                <div ref="stickySystemStartElem" class="overflow-hidden" :style="stickySystemStartStyle" />
                <div class="absolute top-0 pointer-events-none" :style="stickySystemStartFadeStyle" />
            </div>
            <div class="absolute h-full top-0 left-0 overflow-hidden" :class="!horizontal && 'w-full'" ref="markerContainer" :key="scoreKey" :style="markerContainerStyle">
                <template v-if="scoreContainer">
                    <template v-for="(noteGroup, groupIndex) in resolvedNotes" :key="groupIndex">
                        <HighlightedNote v-for="noteId in noteGroup.items" :key="`${noteId}-${noteGroup.color}`" :note-id="noteId" :color="noteGroup.color" :container="scoreContainer" />
                    </template>
                    <template v-for="(sectionGroup, groupIndex) in resolvedSections" :key="groupIndex">
                        <HighlightedSection v-for="section in sectionGroup.items" :key="`${section.startLine}-${section.endLine}-${section.voice}-${section.label?.value}-${sectionGroup.color}-${sectionGroup.outline}-${sectionGroup.fitBoundingBox}`" :start-line="section.startLine" :end-line="section.endLine" :label="section.label" :voice="section.voice" :color="sectionGroup.color" :outline="sectionGroup.outline" :fit-bounding-box="sectionGroup.fitBoundingBox" :container="scoreContainer" />
                    </template>
                    <template v-for="(connectionGroup, groupIndex) in connections" :key="`connection-${groupIndex}`">
                        <HighlightedConnection v-for="(connection, index) in connectionGroup.items" :key="`${index}-${connection.from.line}F${connection.from.voice}-${connection.to.line}F${connection.to.voice}-${connectionGroup.color}`" :from="connection.from" :to="connection.to" :color="connectionGroup.color" :width="connectionGroup.width" :container="scoreContainer" />
                    </template>
                    <template v-for="(lineGroup, groupIndex) in resolvedLines" :key="groupIndex">
                        <HighlightedSection v-for="line in lineGroup.items" :key="`${line.lineNumber}-${line.label?.value}-${lineGroup.color}`" :start-line="line.lineNumber" :end-line="line.lineNumber" :label="line.label" :color="lineGroup.color" :container="scoreContainer" />
                    </template>
                </template>
            </div>
            <div ref="scoreContainer" class="verovio-canvas-container">
                <VerovioCanvas v-if="verovioCanvasAttrs.data" ref="verovioCanvas" v-bind="{ ...$attrs, ...verovioCanvasAttrs }" @score-is-ready="onScoreIsReady" />
            </div>
        </div>
    </div>
</template>

<style scoped>
:deep(.verovio-canvas-horizontal) {
    overflow-y: visible;
}
</style>
