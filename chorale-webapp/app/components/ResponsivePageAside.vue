<script setup>
// Below lg, where UPageAside is hidden, a sticky bar opens the navigation and the table of contents
// in drawers. The root must be a direct child of UPage for the bar to stick, and -mt-8 undoes the
// padding of <main> in the default layout.
defineProps({
    title: { type: String, required: true },
    tocLinks: { type: Array, default: () => [] },
});

const { t } = useI18n();
const route = useRoute();
const navigationOpen = ref(false);
const tocOpen = ref(false);

watch(() => route.path, () => {
    navigationOpen.value = false;
});
</script>

<template>
    <div class="max-lg:sticky max-lg:top-0 max-lg:z-10 max-lg:-mt-8">
        <UPageAside>
            <slot />
        </UPageAside>
        <div class="-mx-4 flex items-center justify-between border-b border-dashed border-default bg-default/75 px-2 py-2 backdrop-blur sm:-mx-6 sm:px-4 lg:hidden">
            <UDrawer
                v-model:open="navigationOpen"
                direction="left"
                :title="title"
                :handle="false"
                inset
                :ui="{ content: 'w-5/6 max-w-sm' }"
            >
                <UButton :label="title" icon="lucide:text-align-start" color="neutral" variant="ghost" />
                <template #body>
                    <slot />
                </template>
            </UDrawer>

            <UDrawer
                v-if="tocLinks.length"
                v-model:open="tocOpen"
                direction="right"
                :title="t('learnToc')"
                :handle="false"
                inset
                :ui="{ content: 'w-5/6 max-w-sm' }"
            >
                <UButton :label="t('learnToc')" trailing-icon="lucide:chevron-right" color="neutral" variant="ghost" />
                <template #body>
                    <UContentToc
                        :links="tocLinks"
                        open
                        highlight
                        :ui="{ root: 'static mx-0 px-0 sm:mx-0 sm:px-0 bg-transparent backdrop-blur-none', container: 'pt-0 sm:pt-0 border-b-0', trigger: 'hidden' }"
                        @move="tocOpen = false"
                    />
                </template>
            </UDrawer>
        </div>
    </div>
</template>
