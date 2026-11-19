<script setup>
const localePath = useLocalePath();
const route = useRoute();
const { params: { id } } = route;

const { data: chorale } = await useAsyncData(`bach-chorales/${id}`, () => queryCollection('bachChorales').where('stem', '=', `bach-chorales/${id}`).first());

if (!chorale.value) {
    throw createError({
        statusCode: 404,
        statusMessage: 'Page Not Found',
    });
}

// "12. Ach Gott und Herr", or the ID when the chorale has no title.
const heading = computed(() => chorale.value.title ? [chorale.value.number, chorale.value.title].filter(Boolean).join('. ') : id);

useHead({
    title: computed(() => chorale.value.title ? `${heading.value} (${id})` : id),
});

const { filter, listFilter, isFiltered } = useChoraleListFilter();
const isFilterModalOpen = ref(false);
const isFilterPopoverOpen = ref(false);
const { data: allChorales } = await useAsyncData('chorale-list-filter-fields', () => {
    return queryCollection('bachChorales').select('choraleId', 'bwv', 'title', 'key', 'mode', 'cantusFirmus', 'meters').order('stem', 'ASC').all();
});
const filteredChorales = computed(() => (allChorales.value ?? []).filter((item) => matchesChoraleListFilter(item, listFilter.value)));
const positionInFilter = computed(() => {
    const index = filteredChorales.value.findIndex((item) => item.choraleId === id);
    return index === -1 ? null : index + 1;
});
// The neighbours within the filtered chorale list, even if this chorale is not in it.
const surroundingChorales = computed(() => {
    const chorales = (allChorales.value ?? []).filter((item) => item.choraleId === id || matchesChoraleListFilter(item, listFilter.value));
    const index = chorales.findIndex((item) => item.choraleId === id);
    return { prev: chorales[index - 1] ?? null, next: chorales[index + 1] ?? null };
});
const prevChorale = computed(() => surroundingChorales.value.prev);
const nextChorale = computed(() => surroundingChorales.value.next);
const choraleRoute = (item) => item && localePath({ name: 'chorale-bach-id', params: { id: item.choraleId }, hash: route.hash });
const prevChoraleRoute = computed(() => choraleRoute(prevChorale.value));
const nextChoraleRoute = computed(() => choraleRoute(nextChorale.value));

const scoreOptions = useScoreOptions();

// A link can open the score with its filters set, as an exercise of a lesson does:
// ?filters=hideMiddleVoices,intervallsatz turns those on and every other filter off. A filter
// with a value, like the transposition, cannot be turned on like that.
const filters = String(useRoute().query.filters ?? '').split(',').filter((filter) => filter in scoreOptions.humdrumFilterDefinitions && typeof scoreOptions[filter] === 'boolean');
function applyLinkedFilters() {
    scoreOptions.resetHumdrumFilters();
    for (const filter of filters) scoreOptions[filter] = true;
}
if (filters.length) {
    applyLinkedFilters();
    // Again once the stored options are restored, which happens after hydration, see persist-stores.client.js.
    onNuxtReady(applyLinkedFilters);
}

// A link from the search results highlights its matches: ?matches=4:12-15~1:12-14,4:30-31 is a
// voice and its lines for each section, joined with "~" to the sections it links in other voices,
// which are drawn as frames; t1:27-38 is the text of verse 1 over those lines (see SearchResults.vue).
function sectionOf(text) {
    const [, type, number, startLine, endLine] = text.match(/^(t?)(\d+):(\d+)-(\d+)$/) ?? [];
    if (!number) return null;
    const lines = { startLine: Number(startLine), endLine: Number(endLine) };
    return type ? { type: 'text', verse: Number(number), ...lines } : { voice: Number(number), ...lines };
}
const matches = String(useRoute().query.matches ?? '').split(',').flatMap((match) => {
    const [section, ...linkedSections] = match.split('~').map(sectionOf);
    return section ? [{ ...section, linkedSections: linkedSections.filter(Boolean) }] : [];
});
// The lyrics show for a match of them.
if (matches.some((match) => match.type === 'text' || match.linkedSections.some((linked) => linked.type === 'text'))) scoreOptions.showLyrics = true;

// The modulations are annotated on the page, see ChoraleView. The state carries over to the next chorale.
const annotating = useState('annotating', () => false);

// The key as the score writes it, e.g. "G major" or "f# minor".
const keyName = computed(() => [chorale.value.key, chorale.value.mode].filter(Boolean).join(' '));
const subtitle = computed(() => [chorale.value.bwv, keyName.value, chorale.value.meters?.join(' ')].filter(Boolean).join(' · '));
</script>

<template>
    <UContainer>
        <div class="flex flex-col gap-8">
            <ChoraleHeader :collection-id="BACH_CHORALES.id" :chorale-id="id" :heading="heading" :subtitle="subtitle" :prev-route="prevChoraleRoute" :next-route="nextChoraleRoute">
                <template #navigation>
                    <UFieldGroup size="xs">
                        <UPopover mode="hover" :open="isFiltered && isFilterPopoverOpen" @update:open="isFilterPopoverOpen = $event">
                            <UButton
                                :label="$t('filterChoraleList')"
                                :color="isFiltered ? 'primary' : 'neutral'"
                                :variant="isFiltered ? 'soft' : 'outline'"
                                icon="lucide:funnel"
                                @click="isFilterModalOpen = true"
                            />
                            <template #content>
                                <ChoraleListFilterBadges class="p-2" />
                            </template>
                        </UPopover>
                        <UButton
                            v-if="isFiltered"
                            color="primary"
                            variant="soft"
                            icon="lucide:x"
                            :aria-label="$t('resetFilter')"
                            class="px-1.5"
                            :ui="{ leadingIcon: 'size-3' }"
                            @click="filter = ''"
                        />
                    </UFieldGroup>
                </template>
                <div v-if="allChorales.length" class="text-sm text-muted">
                    <template v-if="!isFiltered">{{ $t('choralePosition', { position: positionInFilter, count: filteredChorales.length }) }}</template>
                    <template v-else-if="positionInFilter">{{ $t('choralePositionFiltered', { position: positionInFilter, count: filteredChorales.length }, filteredChorales.length) }}</template>
                    <template v-else>{{ $t('choraleNotInFilter', { count: filteredChorales.length }, filteredChorales.length) }}</template>
                </div>
                <UModal v-model:open="isFilterModalOpen" :title="$t('filterChoraleList')">
                    <template #body>
                        <ChoraleListFilter :count="filteredChorales.length" />
                    </template>
                </UModal>
            </ChoraleHeader>

            <ChoraleView v-model:annotating="annotating" :chorale="chorale" :matches="matches" :prev-route="prevChoraleRoute" :next-route="nextChoraleRoute" />
        </div>
    </UContainer>
</template>
