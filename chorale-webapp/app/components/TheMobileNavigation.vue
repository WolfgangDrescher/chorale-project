<script setup>
const { t } = useI18n();
const route = useRoute();
const items = useNavigationItems();
const open = ref(false);

watch(() => route.path, () => {
    open.value = false;
});
</script>

<template>
    <USlideover v-model:open="open" title="Chorale Project">
        <UButton
            icon="lucide:menu"
            :aria-label="t('navigation')"
            variant="ghost"
            color="neutral"
            square
            class="md:hidden"
            :ui="{ base: 'text-white hover:text-black' }"
        />
        <template #body>
            <UNavigationMenu :items="items" orientation="vertical" />
        </template>
    </USlideover>
</template>
