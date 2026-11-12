<script setup>
// A key in its absolute color, named in the language of the page: "Es-Dur", "e-phrygisch". A
// cantus firmus in another key follows it in a gray part of its own, as short as possible: "dor."
// on the same tonic. Phones leave that part out. Each part names its key in full in a popover.
const props = defineProps({
    // { key, mode, signature } as chorale-info writes it.
    keyContext: { type: Object, required: true },
    cantusFirmus: { type: Object, default: null },
});

const POPOVER_CONTENT = { side: 'bottom', align: 'center' };

const { t, locale } = useI18n();

// The name of a mode and its short form.
const MODE_NAMES = {
    major: ['modeMajor', 'modeMajor'],
    minor: ['modeMinor', 'modeMinor'],
    dorian: ['modeDorian', 'modeDorianShort'],
    phrygian: ['modePhrygian', 'modePhrygianShort'],
    lydian: ['modeLydian', 'modeLydianShort'],
    mixolydian: ['modeMixolydian', 'modeMixolydianShort'],
    locrian: ['modeLocrian', 'modeLocrianShort'],
};

const nameOf = ({ key, mode }) => t('keyWithMode', { key: localizedKeyName(key, locale.value), mode: t(MODE_NAMES[mode][0]) });

const cantusFirmusLabel = computed(() => {
    const { key, mode } = props.cantusFirmus;
    const shortMode = t(MODE_NAMES[mode][1]);
    return key.toLowerCase() === props.keyContext.key.toLowerCase() ? shortMode : `${localizedKeyName(key, locale.value)} ${shortMode}`;
});
</script>

<template>
    <span class="inline-flex overflow-hidden rounded text-sm font-medium leading-tight whitespace-nowrap">
        <UPopover mode="hover" arrow :content="POPOVER_CONTENT">
            <span class="px-1.5 py-0.5" :class="keyAppearance(keyContext).class" :style="keyAppearance(keyContext).style">
                {{ nameOf(keyContext) }}
            </span>
            <template #content>
                <div class="flex gap-2 px-3 py-1.5 text-sm">
                    <span class="text-muted">{{ $t('startingKey') }}</span>
                    <span class="font-medium">{{ nameOf(keyContext) }}</span>
                </div>
            </template>
        </UPopover>
        <UPopover v-if="cantusFirmus" mode="hover" arrow :content="POPOVER_CONTENT">
            <span class="hidden sm:inline px-1 py-0.5 text-xs/[1.4] bg-elevated text-muted">
                {{ cantusFirmusLabel }}
            </span>
            <template #content>
                <div class="flex gap-2 px-3 py-1.5 text-sm">
                    <span class="text-muted">{{ $t('cantusFirmus') }}</span>
                    <span class="font-medium">{{ nameOf(cantusFirmus) }}</span>
                </div>
            </template>
        </UPopover>
    </span>
</template>
