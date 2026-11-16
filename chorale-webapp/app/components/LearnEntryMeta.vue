<script setup>
defineProps({
    entry: { type: Object, required: true },
    lessonNumbers: { type: Array, default: null },
    showDate: { type: Boolean, default: false },
});

const { t, d } = useI18n();
</script>

<template>
    <span class="mt-2 flex flex-wrap items-center gap-1.5">
        <span v-if="showDate && entry.date" class="mr-1 text-xs text-muted">{{ d(new Date(entry.date), LEARN_DATE_FORMAT) }}</span>
        <UBadge v-for="tag in entry.tags" :key="tag" :label="tag" color="neutral" variant="subtle" />
        <span v-if="lessonNumbers" class="ml-1 inline-flex items-center gap-1 text-xs text-muted">
            <UIcon :name="LEARN_PARTS.lessons.icon" class="size-3.5" :class="LEARN_PARTS.lessons.color" />
            {{ t('learnInLessons', { numbers: lessonNumbers.join(', ') }) }}
        </span>
    </span>
</template>
