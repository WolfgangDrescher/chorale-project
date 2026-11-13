<script setup>
// The score on GitHub and in the Verovio Humdrum Viewer. The page adds its own links in the slot.
defineProps({
    githubUrl: { type: String, required: true },
    vhvUrl: { type: String, required: true },
});
</script>

<template>
    <UFieldGroup>
        <UTooltip :text="$t('github')">
            <UButton :to="githubUrl" target="_blank" icon="simple-icons:github" color="neutral" variant="outline" :aria-label="$t('github')" />
        </UTooltip>
        <UTooltip :text="$t('vhv')">
            <UButton :to="vhvUrl" target="_blank" icon="custom:vhv" color="neutral" variant="outline" :aria-label="$t('vhv')" />
        </UTooltip>
        <slot />
    </UFieldGroup>
</template>
