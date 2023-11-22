<script setup>
const props = defineProps({
    direction: Boolean,
    rest: Boolean,
    quality: {
        type: Boolean,
        default: true,
    },
});

const { t } = useI18n();
const items = [];

if (props.quality) {
    items.push({
        term: 'm',
        detail: t('minorInterval'),
    },
    {
        term: 'M',
        detail: t('majorInterval'),
    },
    {
        term: 'P',
        detail: t('perfectInterval'),
    },
    {
        term: 'd',
        detail: t('diminishedInterval'),
    },
    {
        term: 'A',
        detail: t('augmentedInterval'),
    });
}

if (props.direction) {
    items.push({
        term: '+',
        detail: t('ascending'),
    }, {
        term: '-',
        detail: t('descending'),
    });
}

if (props.rest) {
    items.push({
        term: 'r',
        detail: t('rest'),
    });
}
</script>

<template>
    <DescriptionList :items="items" class="text-xs">
        <template v-slot:term="{ item }">
            <div class="flex justify-center">
                <code class="text-xs bg-gray-100 rounded p-1">
                    {{ item.term }}
                </code>
            </div>
        </template>
        <template v-slot:detail="{ item }">
            {{ item.detail }}
        </template>
    </DescriptionList>
</template>
