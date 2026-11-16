<script setup>
const { t } = useI18n();
const route = useRoute();

const { data } = await useAsyncData('learn-navigation', async () => {
    const [lessons, faq] = await Promise.all([
        queryCollection('lessons').where('lessonNumber', 'IS NOT NULL').order('lessonNumber', 'ASC').select('path', 'title', 'lessonNumber').all(),
        queryCollection('faq').order('title', 'ASC').select('path', 'title').all(),
    ]);
    return { lessons, faq };
});

// Also true for the pages below a lesson (its data).
const isAt = (path) => route.path === path || route.path.startsWith(`${path}/`);

const groups = computed(() => [
    { title: t('lessons'), path: '/lessons', ...LEARN_PARTS.lessons, pages: (data.value?.lessons ?? []).map((lesson) => ({ title: `${lesson.lessonNumber}. ${lesson.title}`, path: lesson.path })) },
    { title: t('faq'), path: '/faq', ...LEARN_PARTS.faq, pages: (data.value?.faq ?? []).map(({ title, path }) => ({ title, path })) },
]);

const open = ref(Object.fromEntries(groups.value.map((group) => [group.path, isAt(group.path)])));
watch(() => route.path, () => {
    for (const group of groups.value) if (isAt(group.path)) open.value[group.path] = true;
});
</script>

<template>
    <nav class="text-sm" :aria-label="t('learn')">
        <ul class="space-y-1">
            <li v-for="group in groups" :key="group.path">
                <div class="flex items-center gap-1">
                    <NuxtLink
                        :to="group.path"
                        class="flex min-w-0 grow items-center gap-2 rounded-md px-2 py-1.5 font-semibold hover:bg-elevated"
                        :class="route.path === group.path ? 'text-primary' : 'text-highlighted'"
                    >
                        <UIcon :name="group.icon" class="size-5 shrink-0" :class="group.color" />
                        <span class="truncate">{{ group.title }}</span>
                        <span class="ml-auto text-xs font-normal tabular-nums text-dimmed">{{ group.pages.length }}</span>
                    </NuxtLink>
                    <UButton
                        :icon="open[group.path] ? 'lucide:chevron-up' : 'lucide:chevron-down'"
                        color="neutral"
                        variant="ghost"
                        size="xs"
                        :aria-label="t(open[group.path] ? 'learnCloseGroup' : 'learnOpenGroup', { group: group.title })"
                        :aria-expanded="open[group.path]"
                        @click="open[group.path] = !open[group.path]"
                    />
                </div>
                <ul v-if="open[group.path]" class="mb-2 ml-4.5 border-l border-default">
                    <li v-for="page in group.pages" :key="page.path">
                        <NuxtLink
                            :to="page.path"
                            :title="page.title"
                            class="-ml-px block truncate border-l py-1.5 pl-4 pr-2"
                            :class="isAt(page.path) ? 'border-primary font-medium text-primary' : 'border-transparent text-muted hover:border-accented hover:text-default'"
                        >
                            {{ page.title }}
                        </NuxtLink>
                    </li>
                </ul>
            </li>
        </ul>
    </nav>
</template>
