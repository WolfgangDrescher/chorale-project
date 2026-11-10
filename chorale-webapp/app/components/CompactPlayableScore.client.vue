<script setup>
// A small score with its own player, for examples side by side.
// The padding keeps the progress ring clear of the clipping of a carousel.
defineOptions({ inheritAttrs: false });

const player = useScorePlayer();
</script>

<template>
    <div class="flex flex-col gap-1">
        <div class="flex items-center gap-2 p-1.5">
            <ScorePlayerControls :player="player" compact class="shrink-0 scale-75 -m-1" />
            <slot />
        </div>
        <PlayableHighlightedScore
            :player="player"
            :horizontal="true"
            :verovio-options="{ scale: 35, pageMarginLeft: 42 }"
            v-bind="$attrs"
        />
    </div>
</template>
