<script setup>
const localePath = useLocalePath();
const { params: { id } } = useRoute();

const { data: chorale } = await useAsyncData(`chorales/${id}`, () => queryCollection('chorales').where('stem', '=', `chorales/${id}`).first());

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
    return queryCollection('chorales').select('choraleId', 'bwv', 'title', 'key', 'mode', 'cantusFirmus', 'meters').order('stem', 'ASC').all();
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

const { githubScoreUrlGenerator, vhvScoreUrlGenerator } = useScoreUrlGenerator();

const scoreOptions = useScoreOptions();

// A link can open the score with its filters set, as an exercise of a lesson does:
// ?filters=hideMiddleVoices,intervallsatz turns those on and every other filter off. A filter
// with a value, like the transposition, cannot be turned on like that.
const filters = String(useRoute().query.filters ?? '').split(',').filter((filter) => filter in scoreOptions.humdrumFilterDefinitions && typeof scoreOptions[filter] === 'boolean');
if (filters.length) {
    scoreOptions.resetHumdrumFilters();
    for (const filter of filters) scoreOptions[filter] = true;
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

// The downloads of a chorale, or of a worksheet with only the cantus firmus written out: as a PDF to print or as MEI for notation programs.
function useChoraleExport() {
    const { t } = useI18n();

    const exportFormat = ref('pdf');
    const exportContent = ref('cantus-firmus');
    const exportVoices = ref('4');
    const exportLayout = ref('piano');
    const pageOrientation = ref('portrait');

    const exportFormatItems = [
        { label: 'PDF', value: 'pdf' },
        { label: 'MEI', value: 'mei' },
    ];
    const exportContentItems = computed(() => [
        { label: t('cantusFirmusOnly'), value: 'cantus-firmus' },
        { label: t('completeChorale'), value: 'complete' },
    ]);
    const exportVoicesItems = computed(() => [
        { label: t('fourVoices'), value: '4' },
        { label: t('twoVoices'), value: '2' },
    ]);
    const exportLayoutItems = computed(() => [
        { label: t('scoreLayout'), value: 'score' },
        { label: t('pianoSetting'), value: 'piano' },
    ]);
    const pageOrientationItems = computed(() => [
        { label: t('portrait'), value: 'portrait' },
        { label: t('landscape'), value: 'landscape' },
    ]);

    // The URL and the file name of a download. The number of voices is a matter of the MEI only,
    // the page orientation of the PDF.
    function exportLink({ format, content, layout, voices, orientation }) {
        const isMei = format === 'mei';
        const query = new URLSearchParams({ format, content, layout });
        if (isMei) query.set('voices', voices);
        else query.set('orientation', orientation);
        return {
            to: `/api/chorales/${chorale.value.choraleId}/export?${query}`,
            download: `${chorale.value.choraleId}-${content}-${layout}-${isMei ? voices : orientation}.${format}`,
        };
    }

    // Every combination there is, for trying them all out.
    function allExportLinks() {
        const links = [];
        for (const content of ['cantus-firmus', 'complete']) {
            for (const layout of ['score', 'piano']) {
                for (const orientation of ['portrait', 'landscape']) links.push(exportLink({ format: 'pdf', content, layout, orientation }));
                links.push(exportLink({ format: 'mei', content, layout, voices: '4' }));
                links.push(exportLink({ format: 'mei', content, layout, voices: '2' }));
            }
        }
        return links;
    }

    // All of them in one ZIP file, which a browser takes without asking, as opposed to a dozen
    // downloads that start at once. What does not come out right is in it as a text file.
    async function downloadAll() {
        const { zipSync, strToU8 } = await import('fflate');
        const files = {};
        for (const { to, download } of allExportLinks()) {
            try {
                const response = await fetch(to);
                if (response.ok) {
                    files[download] = new Uint8Array(await response.arrayBuffer());
                } else {
                    files[`${download}.error.txt`] = strToU8(`${response.status} ${response.statusText}\n${await response.text()}`);
                }
            } catch (error) {
                files[`${download}.error.txt`] = strToU8(String(error));
            }
        }
        const url = URL.createObjectURL(new Blob([zipSync(files)], { type: 'application/zip' }));
        const link = document.createElement('a');
        link.href = url;
        link.download = `${chorale.value.choraleId}-all.zip`;
        link.click();
        URL.revokeObjectURL(url);
    }

    const downloadItems = computed(() => {
        const link = exportLink({
            format: exportFormat.value,
            content: exportContent.value,
            layout: exportLayout.value,
            voices: exportVoices.value,
            orientation: pageOrientation.value,
        });
        const items = [[{
            label: t('downloadFormat', { format: exportFormat.value.toUpperCase() }),
            icon: 'lucide:download',
            external: true,
            ...link,
        }]];
        if (import.meta.dev) {
            items.push([{ label: t('downloadAll'), icon: 'lucide:bug', onSelect: downloadAll }]);
        }
        return items;
    });

    return {
        exportFormat,
        exportContent,
        exportVoices,
        exportLayout,
        pageOrientation,
        exportFormatItems,
        exportContentItems,
        exportVoicesItems,
        exportLayoutItems,
        pageOrientationItems,
        downloadItems,
    };
}

const {
    exportFormat,
    exportContent,
    exportVoices,
    exportLayout,
    pageOrientation,
    exportFormatItems,
    exportContentItems,
    exportVoicesItems,
    exportLayoutItems,
    pageOrientationItems,
    downloadItems,
} = useChoraleExport();

const player = useScorePlayer();
useScorePlayerKeyboardShortcuts(() => toggleCurrentScorePlayer(player));

const modulations = computed(() => chorale.value.modulations ?? []);

const modulationLines = computed(() => [{
    color: 'rgb(34 197 94 / 0.4)',
    items: modulations.value.map(({ line, key, mode }) => ({
        lineNumber: line,
        label: { value: !mode || ['major', 'minor'].includes(mode) ? key : `${key}:${mode.slice(0, 3)}`, position: 'bottom' },
    })),
}]);

// The modulations are annotated on the page, see useModulationAnnotator; the development server
// writes them to the file. The state carries over to the next chorale.
const annotating = useState('annotating', () => false);
const annotator = useModulationAnnotator(id, player, annotating, import.meta.dev);

useChoraleKeyboardShortcuts({ prevChorale, nextChorale, keysTaken: () => annotator.active });

// The key as the score writes it, e.g. "G major" or "f# minor".
const keyName = computed(() => [chorale.value.key, chorale.value.mode].filter(Boolean).join(' '));
</script>

<template>
    <UContainer>
        <div class="flex flex-col gap-8">
            <div>
                <Heading>
                    {{ heading }}
                    <div class="text-base font-normal">
                        {{ [chorale.composer, chorale.bwv, keyName, chorale.meters?.join(' ')].filter(Boolean).join(' · ') }}
                    </div>
                </Heading>
                <div class="flex flex-wrap items-center gap-2">
                    <div class="flex gap-2">
                        <UButton :disabled="!prevChorale" :to="localePath({ name: 'chorale-id', params: { id: prevChorale?.choraleId }, hash: $route.hash })" color="neutral" variant="outline" size="xs">
                            <template #leading>
                                <UKbd color="neutral" class="hidden sm:inline-flex">
                                    <UIcon name="lucide:arrow-left" />
                                </UKbd>
                            </template>
                            {{ $t('previousChorale') }}
                        </UButton>
                        <UButton :disabled="!nextChorale" :to="localePath({ name: 'chorale-id', params: { id: nextChorale?.choraleId }, hash: $route.hash })" color="neutral" variant="outline" size="xs">
                            {{ $t('nextChorale') }}
                            <template #trailing>
                                <UKbd color="neutral" class="hidden sm:inline-flex">
                                    <UIcon name="lucide:arrow-right" />
                                </UKbd>
                            </template>
                        </UButton>
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
                    </div>
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
                </div>
            </div>

            <div class="flex flex-wrap items-center gap-x-6 gap-y-3">
                <ScoreOptionsPalette :chorale="chorale" />
                <ScorePlayerControls :player="player" :file-name="chorale.choraleId" />
                <div class="shrink-0 flex gap-2 ml-auto">
                    <UFieldGroup>
                        <UTooltip :text="$t('github')">
                            <UButton :to="githubScoreUrlGenerator(chorale.choraleId)" target="_blank" icon="simple-icons:github" color="neutral" variant="outline" :aria-label="$t('github')" />
                        </UTooltip>
                        <UTooltip :text="$t('vhv')">
                            <UButton :to="vhvScoreUrlGenerator(chorale.choraleId)" target="_blank" icon="custom:vhv" color="neutral" variant="outline" :aria-label="$t('vhv')" />
                        </UTooltip>
                        <UTooltip v-if="chorale.urlScan" :text="$t('scan')">
                            <UButton :to="chorale.urlScan" target="_blank" icon="lucide:book-image" color="neutral" variant="outline" :aria-label="$t('scan')" />
                        </UTooltip>
                    </UFieldGroup>
                    <UDropdownMenu :items="downloadItems" arrow>
                        <UButton icon="lucide:download" trailing-icon="lucide:chevron-down" color="neutral" variant="outline" :aria-label="$t('download')" />
                        <template #content-top>
                            <div class="flex flex-col gap-2 p-1.5 pb-1">
                                <div>
                                    <div class="text-xs text-muted mb-1">
                                        {{ $t('exportFormat') }}
                                    </div>
                                    <UTabs v-model="exportFormat" :items="exportFormatItems" :content="false" size="xs" class="w-full" />
                                </div>
                                <div>
                                    <div class="text-xs text-muted mb-1">
                                        {{ $t('exportContent') }}
                                    </div>
                                    <UTabs v-model="exportContent" :items="exportContentItems" :content="false" size="xs" class="w-full" />
                                </div>
                                <div v-if="exportFormat === 'mei'">
                                    <div class="text-xs text-muted mb-1">
                                        {{ $t('exportVoices') }}
                                    </div>
                                    <UTabs v-model="exportVoices" :items="exportVoicesItems" :content="false" size="xs" class="w-full" />
                                </div>
                                <div>
                                    <div class="text-xs text-muted mb-1">
                                        {{ $t('exportLayout') }}
                                    </div>
                                    <UTabs v-model="exportLayout" :items="exportLayoutItems" :content="false" size="xs" class="w-full" />
                                </div>
                                <div v-if="exportFormat === 'pdf'">
                                    <div class="text-xs text-muted mb-1">
                                        {{ $t('pageOrientation') }}
                                    </div>
                                    <UTabs v-model="pageOrientation" :items="pageOrientationItems" :content="false" size="xs" class="w-full" />
                                </div>
                            </div>
                        </template>
                    </UDropdownMenu>
                </div>
            </div>

            <PlayableHighlightedScore
                :player="player"
                :piece-id="chorale.choraleId"
                :horizontal="scoreOptions.showHorizontalViewMode"
                :verovio-options="{
                    ...scoreOptions.verovioOptions,
                    header: 'none',
                    spacingSystem: 15,
                    pageMarginLeft: 50,
                    pageMarginRight: 0,
                    pageMarginTop: 50,
                    pageMarginBottom: 50,
                }"
                :base-filters="scoreOptions.baseFilters"
                :filters="scoreOptions.humdrumFilters"
                :sections="matches"
                :lines="annotator.active ? annotator.markers : (scoreOptions.showModulations ? modulationLines : undefined)"
                :scroll-to-first-section="matches.length > 0"
            />

            <ModulationAnnotator v-if="annotator.active" :annotator="annotator" />

            <ModulationTimeline
                v-model:annotating="annotating"
                :player="player"
                :modulations="annotator.modulations ?? modulations"
                :cantus-firmus="chorale.cantusFirmus"
                :line-shift="lineShiftOf(scoreOptions.humdrumFilters)"
                :outdated="annotator.active && !annotator.saves && annotator.changed"
            />
        </div>
    </UContainer>
</template>
