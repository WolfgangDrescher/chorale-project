<script setup>
// A question before something that cannot be undone, with the button that does it and one that does not.
defineProps({
    question: { type: String, required: true },
    action: { type: String, required: true },
});

const emit = defineEmits(['confirm', 'cancel']);
</script>

<template>
    <div class="flex flex-wrap items-center gap-2">
        <span class="text-sm">{{ question }}</span>
        <UButton color="error" size="sm" :label="action" @click="emit('confirm')" />
        <UButton color="neutral" variant="subtle" size="sm" :label="$t('cancel')" @click="emit('cancel')" />
    </div>
</template>
