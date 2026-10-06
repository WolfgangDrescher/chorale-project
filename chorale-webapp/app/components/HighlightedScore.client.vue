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
});

watch(() => props.scoreData, (data) => {
    if (data) setScore(data, props.filters);
});

onMounted(async () => {
    if (props.scoreData) setScore(props.scoreData, props.filters);
    else loadScore(props.pieceId, props.filters);
    await nextTick();
    updateMarkerWidth();
    setupMutationObserver();
});
</script>

<template>
    <div class="relative" :class="horizontal && 'overflow-x-auto'" ref="wrapperElem" :key="horizontal ? 'horizontal' : 'vertical'" @click="onClick">
        <div class="absolute h-full top-0 left-0 overflow-hidden" :class="!horizontal && 'w-full'" ref="markerContainer" :key="scoreKey" :style="markerContainerStyle">
            <template v-if="scoreContainer">
                <template v-for="(noteGroup, groupIndex) in resolvedNotes" :key="groupIndex">
                    <HighlightedNote v-for="noteId in noteGroup.items" :key="`${noteId}-${noteGroup.color}`" :note-id="noteId" :color="noteGroup.color" :container="scoreContainer" />
                </template>
                <template v-for="(sectionGroup, groupIndex) in resolvedSections" :key="groupIndex">
                    <HighlightedSection v-for="section in sectionGroup.items" :key="`${section.startLine}-${section.endLine}-${section.voice}-${section.label?.value}-${sectionGroup.color}-${sectionGroup.outline}`" :start-line="section.startLine" :end-line="section.endLine" :label="section.label" :voice="section.voice" :color="sectionGroup.color" :outline="sectionGroup.outline" :container="scoreContainer" />
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
</template>

<style scoped>
:deep(.verovio-canvas-horizontal) {
    overflow-y: visible;
}
</style>
