<script setup>
const props = defineProps({
    negative: Boolean,
    nullToken: Boolean,
    accid: {
        type: Boolean,
        default: true,
    },
});

const { t } = useI18n();

const items = [];

if (props.accid) {
    items.push({
        term: '#',
        detail: '♯',
    },
    {
        term: '-',
        detail: '♭',
    },
    {
        term: 'n',
        detail: '♮',
    })
}

if (props.negative) {
    items.push({
        term: '~',
        detail: t('negativeIntervals'),
    });
}

if (props.nullToken) {
    items.push({
        term: '.',
        detail: t('nullToken'),
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
