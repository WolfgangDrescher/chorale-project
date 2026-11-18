<script setup>
const items = useNavigationItems();
</script>

<template>
    <UNavigationMenu :items="items" class="hidden md:flex border-b border-gray-200 dark:border-gray-800 px-2">
        <template #search-content>
            <SearchMenu />
        </template>
        <template #learn-content>
            <LearnMenu />
        </template>
    </UNavigationMenu>
</template>
