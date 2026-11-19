<script>
// In the order of the page, the first is played until another one was used.
const mountedPlayers = new Set();
</script>

<script setup>
const props = defineProps({
    choraleId: { type: String, required: true },
    measures: { type: String, default: '' },
    // Follows the chorale and its measures in the caption.
    caption: { type: String, default: '' },
});

const measuresLabel = computed(() => props.measures && `T. ${props.measures.replace('-', '–')}`);

const { localScoreUrlGenerator } = useScoreUrlGenerator();
const filters = computed(() => (props.measures ? [`myank -m ${props.measures}`] : []));
const player = useScorePlayer();
onMounted(() => mountedPlayers.add(player));
onBeforeUnmount(() => mountedPlayers.delete(player));
useScorePlayerKeyboardShortcuts(() => mountedPlayers.values().next().value);
</script>

<template>
    <figure class="not-prose my-6">
        <div class="mx-auto flex w-fit max-w-full flex-col items-end">
            <ScorePlayerControls :player="player" compact class="transition-opacity duration-300" :class="!player.hasScore && 'opacity-0'" />
            <PlayableHighlightedScore :player="player" :horizontal="true" :score-url="localScoreUrlGenerator(choraleId)" :filters="filters" :verovio-options="{ scale: 40, pageMarginLeft: 42, pageMarginRight: 0 }" />
        </div>
        <figcaption class="mt-2 w-0 min-w-full text-center text-sm text-dimmed text-pretty">
            <Chorale :id="choraleId" />{{ [measuresLabel, caption].filter(Boolean).map((part) => `, ${part}`).join('') }}
        </figcaption>
    </figure>
</template>
