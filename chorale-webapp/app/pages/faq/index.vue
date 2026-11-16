<script setup>
const { t } = useI18n();
</script>

<template>
    <LearnFilteredCollectionPage collection="faq" :title="t('faqTitle')" :description="t('faqDescription')">
        <template #default="{ entries, lessonNumbersBySlug }">
            <LearnEntryList v-slot="{ entry }" :entries="entries">
                <FaqVerdict :verdict="entry.verdict" class="mt-0.5" />
                <span class="min-w-0 grow">
                    <span class="block font-semibold text-highlighted group-hover:text-primary">{{ entry.title }}</span>
                    <span class="mt-0.5 block text-pretty text-default">{{ entry.answer }}</span>
                    <LearnEntryMeta :entry="entry" :lesson-numbers="lessonNumbersBySlug[entry.slug]" show-date />
                </span>
                <LearnStat v-if="entry.stat" :stat="entry.stat" />
            </LearnEntryList>
        </template>
    </LearnFilteredCollectionPage>
</template>
