<script setup>
const props = defineProps({
    id: { type: String, required: true },
    label: { type: String, default: 'Query' },
});

const { t } = useI18n();
const lesson = inject('lesson', null);
const openQuery = inject('openQuery', () => {});
const query = computed(() => lesson?.queries?.find((query) => query.id === props.id));
</script>

<template>
    <UButton
        v-if="query"
        :label="label"
        icon="lucide:search"
        color="neutral"
        variant="soft"
        size="md"
        class="mx-0.5 align-middle"
        :title="query.title"
        :aria-label="t('showQueryTitle', { title: query.title })"
        @click="openQuery(props.id)"
    />
</template>
