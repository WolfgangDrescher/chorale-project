<script setup>
// Title, subtitle and the links to the neighbouring chorales. The page adds its own in the slots.
defineProps({
    heading: { type: String, required: true },
    subtitle: { type: String, default: '' },
    prevRoute: { type: [String, Object], default: null },
    nextRoute: { type: [String, Object], default: null },
});
</script>

<template>
    <div>
        <Heading>
            {{ heading }}
            <div v-if="subtitle" class="text-base font-normal">
                {{ subtitle }}
            </div>
        </Heading>
        <div class="flex flex-wrap items-center gap-2">
            <div class="flex gap-2">
                <UButton :disabled="!prevRoute" :to="prevRoute" color="neutral" variant="outline" size="xs">
                    <template #leading>
                        <UKbd color="neutral" class="hidden sm:inline-flex">
                            <UIcon name="lucide:arrow-left" />
                        </UKbd>
                    </template>
                    {{ $t('previousChorale') }}
                </UButton>
                <UButton :disabled="!nextRoute" :to="nextRoute" color="neutral" variant="outline" size="xs">
                    {{ $t('nextChorale') }}
                    <template #trailing>
                        <UKbd color="neutral" class="hidden sm:inline-flex">
                            <UIcon name="lucide:arrow-right" />
                        </UKbd>
                    </template>
                </UButton>
                <slot name="navigation" />
            </div>
            <slot />
        </div>
    </div>
</template>
