<script setup>
const props = defineProps({
    collection: { type: Object, required: true },
});

const route = useRoute();
const id = route.params.id;

const { data: chorales } = await useAsyncData(`collection-chorales/${props.collection.id}`, () => {
    return queryCollection('collectionChorales').where('collection', '=', props.collection.id).order('stem', 'ASC').all();
});

const index = (chorales.value ?? []).findIndex((item) => item.choraleId === id);
const chorale = chorales.value?.[index];

if (!chorale) {
    throw createError({
        statusCode: 404,
        statusMessage: 'Page Not Found',
    });
}

const heading = `${chorale.number}. ${chorale.title}`;
const subtitle = [chorale.composer, [chorale.key, chorale.mode].filter(Boolean).join(' '), chorale.meters?.join(' ')].filter(Boolean).join(' · ');

useHead({
    title: `${heading} | ${props.collection.name}`,
});

const choraleRoute = useChoraleRoute();
const neighbourRoute = (item) => item && choraleRoute(props.collection.id, item.choraleId, route.hash);
const prevChoraleRoute = neighbourRoute(chorales.value[index - 1]);
const nextChoraleRoute = neighbourRoute(chorales.value[index + 1]);

const scoreOptions = useCollectionScoreOptions(props.collection);
const { localScoreUrlGenerator, githubScoreUrlGenerator, vhvScoreUrlGenerator } = useScoreUrlGenerator(props.collection);

const player = useScorePlayer();
useScorePlayerKeyboardShortcuts(() => toggleCurrentScorePlayer(player));

useChoraleKeyboardShortcuts({ prevRoute: prevChoraleRoute, nextRoute: nextChoraleRoute, scoreOptions });

// The filter groups come first, before any other filter reads the score.
const filters = computed(() => [...filterGroupCommands(props.collection, scoreOptions.filterGroups), ...scoreOptions.humdrumFilters]);

// Not in the template: a page left for the next renders once more, and a new object would reach a
// removed Verovio toolkit.
const verovioOptions = computed(() => ({ ...scoreOptions.verovioOptions, ...CHORALE_PAGE_VEROVIO_OPTIONS }));
</script>

<template>
    <UContainer>
        <div class="flex flex-col gap-8">
            <ChoraleHeader :heading="heading" :subtitle="subtitle" :prev-route="prevChoraleRoute" :next-route="nextChoraleRoute" />

            <div class="flex flex-wrap items-center gap-x-6 gap-y-3">
                <ScoreOptionsPalette :score-options="scoreOptions" />
                <ScorePlayerControls :player="player" :file-name="chorale.choraleId" />
                <ChoraleScoreLinks :github-url="githubScoreUrlGenerator(chorale.choraleId)" :vhv-url="vhvScoreUrlGenerator(chorale.choraleId)" class="shrink-0 ml-auto" />
            </div>

            <ScoreFilterGroup
                v-for="group in collection.filterGroups"
                :key="group.id"
                v-model="scoreOptions.filterGroups[group.id]"
                :group="group"
            />

            <PlayableHighlightedScore
                :player="player"
                :score-url="localScoreUrlGenerator(chorale.choraleId)"
                :horizontal="scoreOptions.showHorizontalViewMode"
                :verovio-options="verovioOptions"
                :base-filters="scoreOptions.baseFilters"
                :filters="filters"
            />
        </div>
    </UContainer>
</template>
