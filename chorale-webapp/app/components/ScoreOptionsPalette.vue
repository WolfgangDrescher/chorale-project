<script setup>
const open = defineModel('open', { type: Boolean });

const props = defineProps({
    // The chorale of the score (an item of the chorales collection), which the options can depend
    // on, e.g. the keys it can be transposed to.
    chorale: { type: Object, default: null },
    // The store of the options, e.g. the one of the search results.
    scoreOptions: { type: Object, default: null },
    size: { type: String, default: 'md' },
});

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

const scoreOptions = props.scoreOptions ?? useScoreOptions();

const showOnlyActive = ref(false);

// The submenus of the filters with a value, by the name of their option: the values to choose
// from (null for the default), how the chosen value is named in the item of the filter, and the
// name of the parameter that stands for the value in the command.
const humdrumFilterSubmenus = {
    transposeKey: useTransposeKeySubmenu(() => props.chorale),
};

// Not given the whole state: the groups would change with every option, and the palette then
// highlights its first item instead of the clicked one.
function humdrumFilterCommand(option, value) {
    return scoreOptions.humdrumFilterDefinitions[option].command({ [option]: value });
}

// The item of a Humdrum filter. A filter without a value is turned on and off. The value of the
// other filters is chosen in a submenu, its search field filters the values by their labels. The
// palette keeps the items of a submenu it navigated to, so whether an item is checked is read
// from the store when the template asks.
function humdrumFilterItem(option) {
    const submenu = humdrumFilterSubmenus[option]?.value;
    const item = {
        label: t(option),
        indicator: 'checkbox',
        checked: () => !!scoreOptions[option],
    };
    if (!submenu) {
        return {
            ...item,
            cmd: scoreOptions.humdrumFilterMap[option],
            onSelect: () => (scoreOptions[option] = !scoreOptions[option]),
        };
    }
    return {
        ...item,
        cmd: scoreOptions[option] ? scoreOptions.humdrumFilterMap[option] : humdrumFilterCommand(option, `{${submenu.parameter}}`),
        suffix: scoreOptions[option] ? submenu.valueLabel(scoreOptions[option]) : undefined,
        placeholder: submenu.placeholder,
        uncheck: () => (scoreOptions[option] = null),
        children: submenu.values.map(({ value, ...child }) => ({
            ...child,
            cmd: value === null ? undefined : humdrumFilterCommand(option, value),
            indicator: 'radio',
            checked: () => scoreOptions[option] === value,
            onSelect: () => (scoreOptions[option] = value),
        })),
    };
}

const keyNotation = useKeyNotation();

// The modulations are turned on and off with the checkbox. The row opens a submenu that hides them
// or shows them with their keys in a notation.
function showModulationsItem() {
    return {
        label: t('showModulations'),
        indicator: 'checkbox',
        checked: () => scoreOptions.showModulations,
        toggle: () => (scoreOptions.showModulations = !scoreOptions.showModulations),
        kbd: 'M',
        suffix: t(KEY_NOTATIONS[keyNotation.value] ?? KEY_NOTATIONS.humdrum),
        placeholder: t('keyNotation'),
        children: [
            {
                label: t('hideModulations'),
                indicator: 'radio',
                checked: () => !scoreOptions.showModulations,
                onSelect: () => (scoreOptions.showModulations = false),
            },
            ...Object.entries(KEY_NOTATIONS).map(([notation, name]) => ({
                label: t(name),
                notation,
                indicator: 'radio',
                checked: () => scoreOptions.showModulations && keyNotation.value === notation,
                onSelect: () => {
                    keyNotation.value = notation;
                    scoreOptions.showModulations = true;
                },
            })),
        ],
    };
}

// A click on the checkbox of a checked filter with a submenu turns it off, one of an item with a
// toggle toggles it; it does not open the submenu, as a click on the rest of the row does.
function onCheckboxClick(event, item) {
    if (item.toggle) {
        event.stopPropagation();
        item.toggle();
        return;
    }
    if (!item.uncheck || !item.checked()) return;
    event.stopPropagation();
    item.uncheck();
}

const groups = computed(() => {
    const allGroups = [
        {
            id: 'humdrum-filters',
            label: t('humdrumFilters'),
            // Without a chorale there is no key to transpose from.
            items: Object.keys(scoreOptions.humdrumFilterDefinitions)
                .filter((option) => props.chorale || option !== 'transposeKey')
                .map(humdrumFilterItem),
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
                    indicator: 'checkbox',
                    checked: () => scoreOptions.showHorizontalViewMode,
                    kbd: 'H',
                },
            ],
        },
        {
            id: 'layers',
            label: t('layers'),
            items: [
                {
                    label: t('showLyrics'),
                    onSelect: () => (scoreOptions.showLyrics = !scoreOptions.showLyrics),
                    indicator: 'checkbox',
                    checked: () => scoreOptions.showLyrics,
                    kbd: 'L',
                },
                // Without a chorale there are no modulations to show.
                ...(props.chorale ? [showModulationsItem()] : []),
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
            items: g.items.filter(item => item.checked?.()),
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
                :size="size"
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
                :ui="{ itemTrailingIcon: 'hidden' }"
            >
                <template #item-leading="{ item }">
                    <span v-if="item.indicator === 'radio'" class="size-4 shrink-0 rounded-full ring ring-inset ring-accented overflow-hidden">
                        <span v-if="item.checked()" class="flex items-center justify-center size-full bg-primary after:size-1.5 after:bg-default after:rounded-full" />
                    </span>
                    <span v-else-if="item.indicator === 'checkbox'" class="flex" @click="onCheckboxClick($event, item)">
                        <UCheckbox :model-value="item.checked()" />
                    </span>
                </template>
                <template #item-description="{ item }">
                    <KeyNotationExamples v-if="item.notation" :notation="item.notation" />
                </template>
                <template #item-trailing="{ item }">
                    <div v-if="item.cmd" class="font-mono text-[0.55rem] text-gray-500" :class="{ 'translate-y-1': !item.children?.length }">{{ item.cmd }}</div>
                    <template v-if="item.kbd">
                        <UKbd v-for="kbd in Array.isArray(item.kbd) ? item.kbd : [item.kbd]" :key="kbd" :value="kbd" size="sm" class="font-mono translate-y-0.5" />
                    </template>
                    <UIcon v-if="item.children?.length" name="i-lucide-chevron-right" class="size-5 shrink-0" />
                </template>
                <template #footer>
                    <UCheckbox v-model="showOnlyActive" size="xs" :label="$t('showOnlyActiveFilters')" />
                </template>
            </UCommandPalette>
        </template>
    </UModal>
</template>
