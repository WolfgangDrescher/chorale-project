<script setup>
const { t } = useI18n();
const localePath = useLocalePath();

const items = [
    {
        label: t('home'),
        to: localePath({name: 'index'}),
    },
];
</script>

<template>
    <UNavigationMenu :items="items" class="border-b border-gray-200 dark:border-gray-800 px-2"/>
</template>
