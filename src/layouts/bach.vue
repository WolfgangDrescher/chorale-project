<script setup>
useHead({
    titleTemplate: (titleChunk) => {
        return titleChunk ? `${titleChunk} | Chorale Project` : 'Chorale Project';
    },
});
</script>

<template>
    <NuxtLayout name="default">
        <template #subnav>
            <NavigationItem :href="localePath('bach')">{{ $t('chorales') }}</NavigationItem>
            <NavigationItem :href="localePath('bach-cadence-degrees')">{{ $t('phraseDegrees') }}</NavigationItem>
            <NavigationItem :href="localePath('bach-phrases')">{{ $t('choralePhrases') }}</NavigationItem>
        </template>
        <NuxtPage></NuxtPage>
    </NuxtLayout>
</template>
