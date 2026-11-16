<script setup>
const props = defineProps({
    choraleId: { type: String, required: true },
    caption: { type: String, default: '' },
    measures: { type: String, default: '' },
});

const { localScoreUrlGenerator } = useScoreUrlGenerator();
const filters = computed(() => (props.measures ? [`myank -m ${props.measures}`] : []));
const player = useScorePlayer();
</script>

<template>
    <figure class="not-prose my-6">
        <div class="mx-auto flex w-fit max-w-full flex-col items-end">
            <ScorePlayerControls :player="player" compact class="transition-opacity duration-300" :class="!player.hasScore && 'opacity-0'" />
            <PlayableHighlightedScore :player="player" :horizontal="true" :score-url="localScoreUrlGenerator(choraleId)" :filters="filters" :verovio-options="{ scale: 40, pageMarginLeft: 42, pageMarginRight: 0 }" />
        </div>
        <figcaption v-if="caption" class="mt-2 text-center text-sm text-dimmed text-pretty">
            <NuxtLink :to="`/chorale/bach/${choraleId}`" class="hover:underline">{{ caption }}</NuxtLink>
        </figcaption>
    </figure>
</template>
