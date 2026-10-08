<script setup>
const open = defineModel('open', { type: Boolean });

onMounted(() => {
    const handler = (e) => {
        if ((e.metaKey || e.ctrlKey) && e.key.toLowerCase() === 'p') {
            e.preventDefault();
            e.stopPropagation();
            open.value = true;
        }
    };
    window.addEventListener('keydown', handler, { capture: true });
    onBeforeUnmount(() => {
        window.removeEventListener('keydown', handler, { capture: true });
    });
});

const { t } = useI18n();

const scoreOptions = useScoreOptions();

const showOnlyActive = ref(false);

const groups = computed(() => {
    const allGroups = [
        {
            id: 'humdrum-filters',
            label: t('humdrumFilters'),
            items: [
                {
                    label: t('showMeter'),
                    cmd: scoreOptions.humdrumFilterMap.showMeter,
                    onSelect: () => (scoreOptions.showMeter = !scoreOptions.showMeter),
                    active: scoreOptions.showMeter,
                },
                {
                    label: t('satb2gs'),
                    cmd: scoreOptions.humdrumFilterMap.satb2gs,
                    onSelect: () => (scoreOptions.satb2gs = !scoreOptions.satb2gs),
                    active: scoreOptions.satb2gs,
                },
                {
                    label: t('intervallsatz'),
                    cmd: scoreOptions.humdrumFilterMap.intervallsatz,
                    onSelect: () => (scoreOptions.intervallsatz = !scoreOptions.intervallsatz),
                    active: scoreOptions.intervallsatz,
                },
                {
                    label: t('extendedFiguredbass'),
                    cmd: scoreOptions.humdrumFilterMap.extendedFiguredbass,
                    onSelect: () => (scoreOptions.extendedFiguredbass = !scoreOptions.extendedFiguredbass),
                    active: scoreOptions.extendedFiguredbass,
                },
                {
                    label: t('bassScaleDegree'),
                    cmd: scoreOptions.humdrumFilterMap.bassScaleDegree,
                    onSelect: () => (scoreOptions.bassScaleDegree = !scoreOptions.bassScaleDegree),
                    active: scoreOptions.bassScaleDegree,
                },
                {
                    label: t('scaleDegree'),
                    cmd: scoreOptions.humdrumFilterMap.scaleDegree,
                    onSelect: () => (scoreOptions.scaleDegree = !scoreOptions.scaleDegree),
                    active: scoreOptions.scaleDegree,
                },
                {
                    label: t('hideMiddleVoices'),
                    cmd: scoreOptions.humdrumFilterMap.hideMiddleVoices,
                    onSelect: () => (scoreOptions.hideMiddleVoices = !scoreOptions.hideMiddleVoices),
                    active: scoreOptions.hideMiddleVoices,
                },
                {
                    label: t('extractCantusFirmus'),
                    cmd: scoreOptions.humdrumFilterMap.extractCantusFirmus,
                    onSelect: () => (scoreOptions.extractCantusFirmus = !scoreOptions.extractCantusFirmus),
                    active: scoreOptions.extractCantusFirmus,
                },
                {
                    label: t('hideNonCantusFirmusVoices'),
                    cmd: scoreOptions.humdrumFilterMap.hideNonCantusFirmusVoices,
                    onSelect: () => (scoreOptions.hideNonCantusFirmusVoices = !scoreOptions.hideNonCantusFirmusVoices),
                    active: scoreOptions.hideNonCantusFirmusVoices,
                },
            ],
        },
        {
            id: 'verovio',
            label: t('verovioOptions'),
            items: [
                {
                    label: t('zoomIn'),
                    icon: 'i-lucide-zoom-in',
                    kbd: '+',
                    onSelect: () => scoreOptions.zoomIn(),
                },
                {
                    label: t('zoomOut'),
                    icon: 'i-lucide-zoom-out',
                    kbd: '-',
                    onSelect: () => scoreOptions.zoomOut(),
                },
                {
                    label: t('resetZoom'),
                    icon: 'i-lucide-image-upscale',
                    kbd: '0',
                    onSelect: () => scoreOptions.resetZoom(),
                },
                {
                    label: t('showHorizontalViewMode'),
                    onSelect: () => (scoreOptions.showHorizontalViewMode = !scoreOptions.showHorizontalViewMode),
                    active: scoreOptions.showHorizontalViewMode,
                    showCheckbox: true,
                    kbd: 'H',
                },
            ],
        },
        {
            id: 'reset',
            label: t('reset'),
            items: [
                {
                    label: t('resetAllVerovioOptions'),
                    onSelect: () => scoreOptions.resetVerovio(),
                },
                {
                    label: t('resetAllHumdrumFilters'),
                    onSelect: () => scoreOptions.resetHumdrumFilters(),
                },
                {
                    label: t('resetAll'),
                    kbd: 'R',
                    onSelect: () => scoreOptions.reset(),
                },
            ],
        },
    ];

    if (showOnlyActive.value) {
        return allGroups.map(g => ({
            ...g,
            items: g.items.filter(item => item.active),
        })).filter(g => g.items.length > 0);
    }

    return allGroups;
});
</script>

<template>
    <UModal v-model:open="open" @after:leave="showOnlyActive = false">
        <UChip :text="scoreOptions.countTotal" :show="scoreOptions.countTotal > 0" size="3xl">
            <UButton
                :label="$t('scoreCommandPalette')"
                variant="soft"
                icon="i-lucide-terminal"
            >
                <template #trailing>
                    <div class="hidden sm:flex gap-1">
                        <UKbd value="meta" />
                        <UKbd color="neutral" class="font-mono">P</UKbd>
                    </div>
                </template>
            </UButton>
        </UChip>

        <template #content>
            <UCommandPalette
                :placeholder="$t('searchCommandsAndScoreOptions')"
                :groups="groups"
            >
                <template #item-leading="{ item }">
                    <UCheckbox v-if="item.group === 'humdrum-filters' || item.showCheckbox" v-model="item.active" />
                </template>
                <template #item-trailing="{ item }">
                    <div v-if="item.cmd" class="font-mono text-[0.55rem] text-gray-500 translate-y-1">{{ item.cmd }}</div>
                    <template v-if="item.kbd">
                        <UKbd v-for="kbd in Array.isArray(item.kbd) ? item.kbd : [item.kbd]" :key="kbd" :value="kbd" size="sm" class="font-mono translate-y-0.5" />
                    </template>
                </template>
                <template #footer>
                    <UCheckbox v-model="showOnlyActive" size="xs" :label="$t('showOnlyActiveFilters')" />
                </template>
            </UCommandPalette>
        </template>
    </UModal>
</template>
