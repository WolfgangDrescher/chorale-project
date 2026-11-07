<script setup>
// The controls of a player made with useScorePlayer: play and pause, the progress (compact as a
// ring around the button, otherwise as a bar), a click on which goes to that point, and, unless
// compact, the time and the settings all players share.
const props = defineProps({
    player: { type: Object, required: true },
    compact: { type: Boolean, default: false },
    fileName: { type: String, default: undefined },
});

// The progress is drawn a little darker than the button, so it stands apart from it.
const PROGRESS_COLOR = 'color-mix(in srgb, var(--ui-primary) 80%, black)';

// The icons are inline SVGs, so their shapes can be filled. The ring is drawn in a view box of 40
// around the button of 28, from under the button outwards; a click on it seeks by its angle.

const progress = computed(() => props.player.duration ? Math.min(props.player.position / props.player.duration, 1) : 0);

function seekToClick(event) {
    const rect = event.currentTarget.getBoundingClientRect();
    props.player.seek(Math.min(Math.max((event.clientX - rect.left) / rect.width, 0), 1) * props.player.duration);
}

function seekToRingClick(event) {
    const rect = event.currentTarget.ownerSVGElement.getBoundingClientRect();
    const angle = Math.atan2(event.clientX - (rect.left + rect.width / 2), (rect.top + rect.height / 2) - event.clientY);
    props.player.seek((angle / (2 * Math.PI) + 1) % 1 * props.player.duration);
}

// "1:05"
function formatTime(time) {
    const seconds = Math.round(props.player.toSeconds(time));
    return `${Math.floor(seconds / 60)}:${String(seconds % 60).padStart(2, '0')}`;
}
</script>

<template>
    <div class="flex items-center gap-3" @pointerdown="player.makeCurrent()" @focusin="player.makeCurrent()">
        <div class="group relative shrink-0" :class="compact ? 'size-7' : 'size-10'">
            <button
                type="button"
                class="relative z-10 size-full inline-flex items-center justify-center rounded-full bg-primary text-inverted hover:bg-primary/85 cursor-pointer disabled:cursor-default disabled:opacity-50 focus-visible:outline-2 focus-visible:outline-offset-2 focus-visible:outline-primary transition-colors"
                :disabled="!player.hasScore"
                :aria-label="player.isPlaying ? $t('playerPause') : $t('playerPlay')"
                @click="player.toggle()"
            >
                <UIcon v-if="player.isLoading" name="lucide:loader-circle" class="size-1/2 animate-spin" />
                <UIcon v-else :name="player.isPlaying ? 'lucide:pause' : 'lucide:play'" mode="svg" class="size-1/2 [&_path]:fill-current [&_rect]:fill-current" :class="!player.isPlaying && 'translate-x-[4%]'" />
            </button>
            <svg
                v-if="compact && player.hasScore"
                viewBox="0 0 40 40"
                class="absolute -inset-1.5 size-10 -rotate-90 pointer-events-none text-primary transition-opacity [&_.ring]:transition-[r,stroke-width] group-hover:[&_.ring]:[r:16.5] group-hover:[&_.ring]:[stroke-width:7px]"
                :class="!progress && 'opacity-0 group-hover:opacity-100'"
            >
                <circle cx="20" cy="20" r="15" fill="none" stroke="currentColor" stroke-opacity="0.2" stroke-width="4" class="ring" />
                <circle cx="20" cy="20" r="15" fill="none" :stroke="PROGRESS_COLOR" stroke-width="4" pathLength="100" :stroke-dasharray="`${progress * 100} 100`" class="ring" />
                <circle cx="20" cy="20" r="17" fill="none" stroke="transparent" stroke-width="6" pointer-events="stroke" class="cursor-pointer" @click="seekToRingClick" />
            </svg>
        </div>
        <template v-if="!compact">
            <div
                class="group w-32 sm:w-44 h-4 flex items-center cursor-pointer"
                role="progressbar"
                :aria-label="$t('playerPosition')"
                aria-valuemin="0"
                :aria-valuemax="Math.round(player.toSeconds(player.duration))"
                :aria-valuenow="Math.round(player.toSeconds(player.position))"
                @click="seekToClick"
            >
                <div class="w-full h-1.5 group-hover:h-2 transition-[height] rounded-full bg-accented overflow-hidden">
                    <div class="h-full rounded-full" :style="{ width: `${progress * 100}%`, backgroundColor: PROGRESS_COLOR }" />
                </div>
            </div>
            <span class="text-xs text-muted tabular-nums whitespace-nowrap">
                {{ formatTime(player.position) }} / {{ formatTime(player.duration) }}
            </span>
            <ScorePlayerSettings :player="player" :file-name="fileName" />
        </template>
    </div>
</template>
