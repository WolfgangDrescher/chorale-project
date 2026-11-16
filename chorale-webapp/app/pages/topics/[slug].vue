<script setup>
const { t } = useI18n();
const localePath = useLocalePath();
</script>

<template>
    <LearnPage collection="topics" :breadcrumb="[{ label: t('learn'), to: localePath('/learn') }, { label: t('topics'), to: localePath('/topics') }]" />
</template>
