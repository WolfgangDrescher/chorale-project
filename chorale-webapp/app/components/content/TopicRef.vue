<script setup>
const props = defineProps({
    id: { type: String, required: true },
});

const { t } = useI18n();
const { data: topic } = await useAsyncData(`topic-ref-${props.id}`, () => {
    return queryCollection('topics').path(`/topics/${props.id}`).select('path', 'title', 'description').first();
});
</script>

<template>
    <NuxtLink v-if="topic" :to="topic.path" class="not-prose group my-6 flex gap-4 rounded-lg border border-default border-l-4 border-l-primary bg-elevated/40 p-4 transition hover:bg-elevated">
        <UIcon :name="LEARN_PARTS.topics.icon" class="mt-1 size-5 shrink-0" :class="LEARN_PARTS.topics.color" />
        <span class="min-w-0 grow">
            <span class="block text-xs font-semibold uppercase tracking-wide text-primary">{{ t('topic') }}</span>
            <span class="block font-semibold text-highlighted group-hover:text-primary">{{ topic.title }}</span>
            <span class="mt-1 block text-pretty text-default">{{ topic.description }}</span>
            <span class="mt-2 inline-flex items-center gap-1 text-sm text-primary">{{ t('topicReadAll') }} <UIcon name="lucide:arrow-right" class="size-4" /></span>
        </span>
    </NuxtLink>
    <UAlert v-else :title="t('referenceMissing', { id })" color="warning" variant="subtle" icon="lucide:unlink" class="my-6" />
</template>
