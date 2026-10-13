<script setup>
import { useClipboard } from '@vueuse/core';

const localePath = useLocalePath();
const { params: { id } } = useRoute();

const { data: chorale } = await useAsyncData(`chorales/${id}`, () => queryCollection('chorales').where('stem', '=', `chorales/${id}`).first());

if (!chorale.value) {
    throw createError({
        statusCode: 404,
        statusMessage: 'Page Not Found',
    });
}

useHead({
    title: computed(() => chorale.value.title ? `${chorale.value.title} (${id})` : id),
});

const { data: surroundData } = await useAsyncData(`chorales/${id}/surround`, () => {
    return queryCollectionItemSurroundings('chorales', chorale.value.path, { fields: ['choraleId'] }).order('stem', 'ASC');
});
const prevChorale = computed(() => surroundData.value?.[0] ?? null);
const nextChorale = computed(() => surroundData.value?.[1] ?? null);

const { githubScoreUrlGenerator, vhvScoreUrlGenerator } = useScoreUrlGenerator();

const scoreOptions = useScoreOptions();

const { copy, copied } = useClipboard();

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

    const downloadItems = computed(() => {
        const link = exportLink({
            format: exportFormat.value,
            content: exportContent.value,
            layout: exportLayout.value,
            voices: exportVoices.value,
            orientation: pageOrientation.value,
        });
        return [[{
            label: t('downloadFormat', { format: exportFormat.value.toUpperCase() }),
            icon: 'lucide:download',
            external: true,
            ...link,
        }]];
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

useChoraleKeyboardShortcuts({ prevChorale, nextChorale });

// The key as the score writes it, e.g. "G major" or "f# minor".
const keyName = computed(() => [chorale.value.key, chorale.value.mode].filter(Boolean).join(' '));
</script>

<template>
    <UContainer>
        <div class="flex flex-col gap-8">
            <div>
                <div class="mb-3">
                    <UBadge color="neutral" size="sm" variant="outline" class="font-mono cursor-pointer select-none w-[11ch] inline-flex items-center justify-center text-center" :label="copied ? $t('copied') : id" @click="copy(id)" />
                </div>
                <Heading>
                    {{ chorale.title ?? id }}
                    <div class="text-base font-normal">
                        {{ [chorale.composer, chorale.bwv, keyName, chorale.meter].filter(Boolean).join(' · ') }}
                    </div>
                </Heading>
                <div class="flex gap-2">
                    <UButton :disabled="!prevChorale" :to="localePath({ name: 'chorale-id', params: { id: prevChorale?.choraleId }, hash: $route.hash })" size="xs">
                        <template #leading>
                            <UKbd color="neutral">
                                <UIcon name="lucide:arrow-left" />
                            </UKbd>
                        </template>
                        {{ $t('previous') }}
                    </UButton>
                    <UButton :disabled="!nextChorale" :to="localePath({ name: 'chorale-id', params: { id: nextChorale?.choraleId }, hash: $route.hash })" size="xs">
                        {{ $t('next') }}
                        <template #trailing>
                            <UKbd color="neutral">
                                <UIcon name="lucide:arrow-right" />
                            </UKbd>
                        </template>
                    </UButton>
                </div>
            </div>

            <div class="flex flex-col md:flex-row items-center gap-4">
                <div>
                    <ScoreOptionsPalette />
                </div>
                <div class="shrink-0 flex gap-2 ml-auto md:order-2">
                    <UButton :to="githubScoreUrlGenerator(chorale.choraleId)" target="_blank">
                        {{ $t('github') }}
                    </UButton>
                    <UButton :to="vhvScoreUrlGenerator(chorale.choraleId)" target="_blank">
                        {{ $t('vhv') }}
                    </UButton>
                    <UDropdownMenu :items="downloadItems" arrow>
                        <UButton trailing-icon="lucide:chevron-down">
                            {{ $t('download') }}
                        </UButton>
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

            <HighlightedScore
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
                :filters="scoreOptions.humdrumFilters"
            />
        </div>
    </UContainer>
</template>
