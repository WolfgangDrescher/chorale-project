<script>
// In the order of the page, the first is played until another one was used.
const mountedPlayers = new Set();
</script>

<script setup>
const props = defineProps({
    // A chorale of the corpus, or a score of its own as **kern.
    choraleId: { type: String, default: '' },
    kern: { type: String, default: '' },
    measures: { type: String, default: '' },
    // Follows the chorale and its measures in the caption.
    caption: { type: String, default: '' },
});

const measuresLabel = computed(() => props.measures && `T. ${props.measures.replace('-', '–')}`);
const captionParts = computed(() => [measuresLabel.value, props.caption].filter(Boolean));
const captionText = computed(() => (props.choraleId ? captionParts.value.map((part) => `, ${part}`).join('') : captionParts.value.join(', ')));

const { localScoreUrlGenerator } = useScoreUrlGenerator();
const scoreSource = computed(() => (props.kern ? { scoreData: props.kern } : { scoreUrl: localScoreUrlGenerator(props.choraleId) }));
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
            <PlayableHighlightedScore :player="player" :horizontal="true" v-bind="scoreSource" :filters="filters" :verovio-options="{ scale: 40, pageMarginLeft: 42, pageMarginRight: 0 }" />
        </div>
        <figcaption v-if="choraleId || captionText" class="mt-2 w-0 min-w-full text-center text-sm text-dimmed text-pretty">
            <Chorale v-if="choraleId" :id="choraleId" />{{ captionText }}
        </figcaption>
    </figure>
</template>
