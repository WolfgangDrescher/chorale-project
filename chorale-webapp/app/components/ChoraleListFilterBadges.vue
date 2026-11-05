<script setup>
const { listFilterBadges } = useChoraleListFilter();
</script>

<template>
    <div class="flex flex-wrap gap-1">
        <UBadge v-for="(badge, index) in listFilterBadges" :key="index" color="neutral" variant="subtle">
            <span class="text-dimmed">{{ badge.label }}</span>
            {{ badge.value }}
        </UBadge>
    </div>
</template>
