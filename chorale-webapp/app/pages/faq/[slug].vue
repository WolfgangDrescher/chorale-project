<script setup>
const { t } = useI18n();
const localePath = useLocalePath();
</script>

<template>
    <LearnPage collection="faq" :breadcrumb="[{ label: t('learn'), to: localePath('/learn') }, { label: t('faq'), to: localePath('/faq') }]" />
</template>
