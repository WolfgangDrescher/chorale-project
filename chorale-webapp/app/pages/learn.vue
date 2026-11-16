<script setup>
const { t } = useI18n();

useSeoMeta({ title: t('learn'), description: t('learnDescription') });
</script>

<template>
    <UContainer>
        <UPageHeader :title="t('learn')" :description="t('learnDescription')" />
        <UPageBody>
            <LearnMenu cards />
        </UPageBody>
    </UContainer>
</template>
