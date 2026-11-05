<script setup>
import { useLocalStorage } from '@vueuse/core';

// The keys of a chorale as segments along its playback.
const props = defineProps({
    player: { type: Object, required: true },
    // { line, key, mode, keyDeg, signature }, the starting key first.
    modulations: { type: Array, required: true },
    // The source's key of the cantus firmus, which may be another than the starting key.
    cantusFirmus: { type: Object, default: null },
    lineShift: { type: Number, default: 0 },
    // Whether there are changes to the modulations the timeline does not show yet.
    outdated: { type: Boolean, default: false },
});

const { t } = useI18n();
const annotating = defineModel('annotating', { type: Boolean, default: false });

// Relative: the starting key always in the color of C, the others by their distance to it.
const coloring = useLocalStorage('chorale-key-coloring', 'absolute');
const coloringItems = computed(() => [
    { label: t('keyColoringAbsolute'), value: 'absolute' },
    { label: t('keyColoringRelative'), value: 'relative' },
]);

const labeling = useLocalStorage('chorale-key-labeling', 'keys');
const labelingItems = computed(() => [
    { label: t('keyLabelingKeys'), value: 'keys' },
    { label: t('keyLabelingDegrees'), value: 'degrees' },
]);

// Tailwind colors around the color wheel, one per fifth, starting at the signature of C. --key-color
// marks the bar of a minor key and the outline of the starting key.
const COLORS_BY_SIGNATURE = [
    { major: 'bg-cyan-500 text-white [--key-color:var(--color-cyan-500)]', minor: 'bg-cyan-100 text-cyan-800 [--key-color:var(--color-cyan-500)]' },
    { major: 'bg-blue-500 text-white [--key-color:var(--color-blue-500)]', minor: 'bg-blue-100 text-blue-800 [--key-color:var(--color-blue-500)]' },
    { major: 'bg-indigo-500 text-white [--key-color:var(--color-indigo-500)]', minor: 'bg-indigo-100 text-indigo-800 [--key-color:var(--color-indigo-500)]' },
    { major: 'bg-violet-500 text-white [--key-color:var(--color-violet-500)]', minor: 'bg-violet-100 text-violet-800 [--key-color:var(--color-violet-500)]' },
    { major: 'bg-pink-500 text-white [--key-color:var(--color-pink-500)]', minor: 'bg-pink-100 text-pink-800 [--key-color:var(--color-pink-500)]' },
    { major: 'bg-red-500 text-white [--key-color:var(--color-red-500)]', minor: 'bg-red-100 text-red-800 [--key-color:var(--color-red-500)]' },
    { major: 'bg-orange-500 text-white [--key-color:var(--color-orange-500)]', minor: 'bg-orange-100 text-orange-800 [--key-color:var(--color-orange-500)]' },
    { major: 'bg-amber-500 text-amber-800 [--key-color:var(--color-amber-500)]', minor: 'bg-amber-100 text-amber-800 [--key-color:var(--color-amber-500)]' },
    { major: 'bg-yellow-500 text-yellow-800 [--key-color:var(--color-yellow-500)]', minor: 'bg-yellow-100 text-yellow-800 [--key-color:var(--color-yellow-500)]' },
    { major: 'bg-lime-500 text-lime-800 [--key-color:var(--color-lime-500)]', minor: 'bg-lime-100 text-lime-800 [--key-color:var(--color-lime-500)]' },
    { major: 'bg-green-500 text-white [--key-color:var(--color-green-500)]', minor: 'bg-green-100 text-green-800 [--key-color:var(--color-green-500)]' },
    { major: 'bg-teal-500 text-white [--key-color:var(--color-teal-500)]', minor: 'bg-teal-100 text-teal-800 [--key-color:var(--color-teal-500)]' },
];

const ROMAN_NUMERALS = ['I', 'II', 'III', 'IV', 'V', 'VI', 'VII'];

// Humdrum writes the tonic of a mode with a minor third in lower case.
const isMinor = (key) => key === key.toLowerCase();
const isChurchMode = (mode) => !['major', 'minor'].includes(mode);

// "D", "f dor", or as a degree of the starting key: "V", "ii", "♭III"; "?" for a key left open.
function labelOf(modulation) {
    const { key, mode, keyDeg } = modulation;
    if (key === '?') return key;
    let label = key.replaceAll('-', '♭').replaceAll('#', '♯');
    if (labeling.value === 'degrees' && keyDeg) {
        const [, number, alteration] = keyDeg.match(/^(\d)([-+]*)$/);
        const roman = ROMAN_NUMERALS[number - 1];
        label = alteration.replaceAll('-', '♭').replaceAll('+', '♯') + (isMinor(key) ? roman.toLowerCase() : roman);
    }
    return isChurchMode(mode) ? `${label} ${mode.slice(0, 3)}` : label;
}

// Major solid, minor light with a bar of its color, church modes hatched, a key left open gray.
function appearanceOf({ key, mode, signature }, homeSignature) {
    if (key === '?') return { class: 'bg-accented text-muted', style: {} };
    const minor = isMinor(key);
    const colors = COLORS_BY_SIGNATURE[(((signature - homeSignature) % 12) + 12) % 12];
    const style = {};
    if (isChurchMode(mode)) {
        const stripe = minor ? 'color-mix(in srgb, var(--key-color) 35%, transparent)' : 'rgb(255 255 255 / 0.3)';
        style.backgroundImage = `repeating-linear-gradient(135deg, ${stripe} 0 3px, transparent 3px 7px)`;
    }
    return { class: minor ? [colors.minor, 'shadow-[inset_0_-4px_0_var(--key-color)]'] : colors.major, style };
}

// The key the score starts in, unless it is left open.
const start = computed(() => (props.modulations[0]?.key === '?' ? null : props.modulations[0]));
const startSignature = computed(() => start.value?.signature ?? 0);
const homeSignature = computed(() => (coloring.value === 'relative' ? startSignature.value : 0));

const segments = computed(() => {
    const { player } = props;
    if (!player.duration || !player.lineStarts.size) return [];
    const changes = props.modulations
        .map((modulation, index) => ({
            modulation,
            start: index === 0 ? 0 : player.lineStarts.get(modulation.line + props.lineShift),
        }))
        .filter(({ start }) => start !== undefined);
    return changes.map(({ modulation, start }, index) => ({
        line: modulation.line,
        start,
        label: labelOf(modulation),
        appearance: appearanceOf(modulation, homeSignature.value),
        width: ((changes[index + 1]?.start ?? player.duration) - start) / player.duration,
    }));
});

// The legend is the line of fifths around the starting key, its tonics in fifths from C.
const LINE_OF_FIFTHS = 'FCGDAEB';
// The signature of a mode in fifths from major on the same tonic; below -1 its third is minor.
const MODE_FIFTHS = { major: 0, minor: -3, dorian: -2, phrygian: -4, lydian: 1, mixolydian: -1, locrian: -5 };

// A key as chorale-info writes it, of the tonic a number of fifths from C: -9 in major is "B--".
function keyAt(fifths, mode) {
    const sharps = Math.floor((fifths + 1) / 7);
    const letter = LINE_OF_FIFTHS[fifths + 1 - 7 * sharps];
    const key = (MODE_FIFTHS[mode] < -1 ? letter.toLowerCase() : letter) + (sharps > 0 ? '#'.repeat(sharps) : '-'.repeat(-sharps));
    return { key, mode, signature: fifths + MODE_FIFTHS[mode] };
}

// The degree of a tonic a number of fifths above the starting tonic, as keyDeg has it: a fifth is
// four steps up, and the scale of the starting mode spans the seven fifths from its fourth on.
function keyDegAt(fifths, startMode) {
    const alteration = Math.floor((fifths - MODE_FIFTHS[startMode] + 1) / 7);
    return `${((4 * fifths % 7) + 7) % 7 + 1}${alteration > 0 ? '+'.repeat(alteration) : '-'.repeat(-alteration)}`;
}

// Six fifths to either side of the starting key, a row with its major or major mode and one with
// its minor or minor mode, as a key and its relative have the same signature.
const legend = computed(() => {
    if (!start.value) return [];
    const { signature, mode: startMode } = start.value;
    const startFifths = signature - MODE_FIFTHS[startMode];
    const rows = MODE_FIFTHS[startMode] < -1 ? ['major', startMode] : [startMode, 'minor'];
    return rows.map((mode) => ({
        label: t(isChurchMode(mode) ? 'keyColorsChurchMode' : mode === 'minor' ? 'keyColorsMinor' : 'keyColorsMajor'),
        entries: Array.from({ length: 13 }, (_, index) => {
            const fifths = signature + index - 6 - MODE_FIFTHS[mode];
            const key = { ...keyAt(fifths, mode), keyDeg: keyDegAt(fifths - startFifths, startMode) };
            return { label: labelOf(key), isHome: index === 6 && mode === startMode, appearance: appearanceOf(key, homeSignature.value) };
        }),
    }));
});

// In the column of its signature, as the starting key is in the middle one; only where it is
// another than the starting key.
const cantusFirmus = computed(() => {
    const key = props.cantusFirmus;
    if (!key || !start.value || (key.key === start.value.key && key.mode === start.value.mode)) return null;
    const column = key.signature - start.value.signature + 6;
    if (column < 0 || column > 12) return null;
    return { label: labelOf(key), appearance: appearanceOf(key, homeSignature.value), column };
});

// The field as wide as the chosen item, its menu as wide as the longest.
const selectWidths = { size: 'xs', variant: 'ghost', content: { align: 'end' }, ui: { content: 'w-max min-w-(--reka-select-trigger-width)' } };

const progress = computed(() => Math.min(props.player.position / props.player.duration, 1));
const showsPlayhead = computed(() => props.player.isPlaying || props.player.soundingIds.length > 0);
</script>

<template>
    <div v-if="segments.length" class="flex flex-col gap-2">
        <div class="relative h-7 select-none">
            <div class="absolute inset-0 flex gap-0.5 rounded-md overflow-hidden">
                <button
                    v-for="segment in segments"
                    :key="segment.line"
                    type="button"
                    class="h-full min-w-0 flex items-center justify-center px-1 text-xs font-semibold whitespace-nowrap overflow-hidden cursor-pointer"
                    :class="segment.appearance.class"
                    :style="{ ...segment.appearance.style, width: `${segment.width * 100}%` }"
                    @click="player.seek(segment.start); $event.currentTarget.blur()"
                >
                    <span class="truncate">{{ segment.label }}</span>
                </button>
            </div>
            <div v-if="showsPlayhead" class="absolute -top-0.5 -bottom-0.5 w-0.5 rounded bg-gray-900 -translate-x-1/2 pointer-events-none" :style="{ left: `${progress * 100}%` }" />
        </div>
        <div class="flex items-center justify-end gap-1">
            <span v-if="outdated" class="mr-auto text-xs text-muted">{{ $t('modulationsUnpublished') }}</span>
            <UButton
                size="xs"
                color="neutral"
                :variant="annotating ? 'soft' : 'ghost'"
                icon="lucide:pencil"
                :label="$t(annotating ? 'annotateDone' : 'annotate')"
                :aria-pressed="annotating"
                @click="annotating = !annotating; $event.currentTarget.blur()"
            />
            <USelect v-model="labeling" :items="labelingItems" v-bind="selectWidths" />
            <USelect v-model="coloring" :items="coloringItems" v-bind="selectWidths" />
            <UPopover>
                <UButton size="xs" color="neutral" variant="ghost" icon="lucide:info" :aria-label="$t('keyColorsExplained')" />
                <template #content>
                    <div class="flex flex-col gap-2 p-3 max-w-xl text-sm">
                        <p>{{ $t(coloring === 'relative' ? 'keyColorsRelativeExplanation' : 'keyColorsAbsoluteExplanation') }} {{ $t('keyColorsOfNeighbors') }}</p>
                        <div class="grid grid-cols-[auto_repeat(13,minmax(0,1fr))] items-center gap-1">
                            <template v-if="cantusFirmus">
                                <span class="col-start-1 pr-1 text-xs text-muted">{{ $t('keyColorsCantusFirmus') }}</span>
                                <div
                                    class="h-6 rounded flex items-center justify-center text-[10px] font-semibold whitespace-nowrap overflow-hidden"
                                    :class="cantusFirmus.appearance.class"
                                    :style="{ ...cantusFirmus.appearance.style, gridColumn: cantusFirmus.column + 2 }"
                                >
                                    {{ cantusFirmus.label }}
                                </div>
                            </template>
                            <template v-for="row in legend" :key="row.label">
                                <span class="col-start-1 pr-1 text-xs text-muted">{{ row.label }}</span>
                                <div
                                    v-for="entry in row.entries"
                                    :key="entry.label"
                                    class="h-6 rounded flex items-center justify-center text-[10px] font-semibold whitespace-nowrap overflow-hidden"
                                    :class="[entry.appearance.class, entry.isHome && 'outline-2 outline-offset-1 outline-(--key-color)']"
                                    :style="entry.appearance.style"
                                >
                                    {{ entry.label }}
                                </div>
                            </template>
                        </div>
                    </div>
                </template>
            </UPopover>
        </div>
    </div>
</template>
