<script setup>
// A HighlightedScore, with its props, that a player made with useScorePlayer plays. The sonority
// at the position of the player is marked, filled while it plays and framed while it pauses, and
// kept in view while it plays. A click on a note goes there: playback goes on from it, or, paused,
// only the frame moves, so a click that means something else on the page does not start sound.
defineOptions({ inheritAttrs: false });

const props = defineProps({
    player: { type: Object, required: true },
});

// The last line one of the sounding notes or rests was attacked on.
const line = computed(() => Math.max(0, ...props.player.soundingIds.map((id) => lineAndVoiceOf(id)?.line ?? 0)) || null);

// The primary color at a share of its strength.
const primaryAt = (percent) => `color-mix(in srgb, var(--ui-primary) ${percent}%, transparent)`;

onBeforeUnmount(() => props.player.disconnect());
</script>

<template>
    <HighlightedScore
        v-bind="$attrs"
        :clickable-notes="true"
        :scroll-to-line="player.isPlaying ? line : undefined"
        @score-loaded="({ callVerovioMethod }) => player.connect(callVerovioMethod)"
        @note-click="({ id }) => player.goTo(id)"
    >
        <template #markers="{ container }">
            <HighlightedSection
                v-if="line"
                :key="`${line}-${player.isPlaying}`"
                :start-line="line"
                :end-line="line"
                :color="primaryAt(player.isPlaying ? 25 : 60)"
                :outline="!player.isPlaying"
                :container="container"
            />
        </template>
    </HighlightedScore>
</template>
