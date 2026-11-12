<script setup>
// The first measures of the cantus firmus, after a pickup if there is one. Until Verovio has
// rendered them, shimmering bars stand in for them instead of its status box. An error stays.
defineProps({
    choraleId: { type: String, required: true },
    measures: { type: Number, default: 5 },
});

const isLoaded = ref(false);

const PLACEHOLDER_WIDTHS = ['w-full', 'w-4/5', 'w-3/5'];
</script>

<template>
    <div class="relative pointer-events-none overflow-hidden mask-r-from-85% [&_.verovio-canvas-status:not(.verovio-canvas-status-error)]:hidden!">
        <div v-if="!isLoaded" class="absolute inset-0 flex flex-col justify-center gap-2">
            <USkeleton
                v-for="width in PLACEHOLDER_WIDTHS"
                :key="width"
                class="h-2 rounded-sm animate-shimmer! bg-size-[200%_100%] bg-linear-to-r from-elevated via-accented to-elevated"
                :class="width"
            />
        </div>
        <HighlightedScore
            :piece-id="choraleId"
            :horizontal="true"
            :filters="[`myank -m 0-${measures}`, 'extract -k $']"
            :verovio-options="{ scale: 30, pageMarginLeft: 10, pageMarginTop: 0, header: 'none', footer: 'none', mnumInterval: 0 }"
            @score-loaded="isLoaded = true"
        />
    </div>
</template>
