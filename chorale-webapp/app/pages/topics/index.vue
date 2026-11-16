<script setup>
const { t } = useI18n();
</script>

<template>
    <LearnFilteredCollectionPage collection="topics" :title="t('topics')" :description="t('topicsDescription')">
        <template #default="{ entries, lessonNumbersBySlug }">
            <LearnEntryList v-slot="{ entry }" :entries="entries">
                <span class="min-w-0 grow">
                    <span class="block font-semibold text-highlighted group-hover:text-primary">{{ entry.title }}</span>
                    <span class="mt-0.5 block text-pretty text-default">{{ entry.description }}</span>
                    <LearnEntryMeta :entry="entry" :lesson-numbers="lessonNumbersBySlug[entry.slug]" />
                </span>
            </LearnEntryList>
        </template>
    </LearnFilteredCollectionPage>
</template>
