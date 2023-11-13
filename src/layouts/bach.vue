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
            <NavigationItem :href="localePath('bach-cadence-degrees')" @click="onClick">{{ $t('cadenceDegrees') }}</NavigationItem>
            <NavigationItem :href="localePath('bach-phrases')" @click="onClick">{{ $t('choralePhrases') }}</NavigationItem>
        </template>
        <NuxtPage></NuxtPage>
    </NuxtLayout>
</template>
