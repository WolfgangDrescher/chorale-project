<script setup>
const { data: lessons } = await useAsyncData('lesson-list', () => {
    return queryCollection('lessons').where('lessonNumber', 'IS NOT NULL').order('lessonNumber', 'ASC').select('path', 'title', 'description', 'lessonNumber').all();
});
</script>

<template>
    <ol class="not-prose my-6 space-y-2">
        <li v-for="lesson in lessons" :key="lesson.path">
            <NuxtLink :to="lesson.path" class="group flex gap-4 py-2">
                <span class="w-6 shrink-0 text-right font-semibold tabular-nums text-primary">{{ lesson.lessonNumber }}</span>
                <span>
                    <span class="block font-semibold text-highlighted group-hover:text-primary">{{ lesson.title }}</span>
                    <span v-if="lesson.description" class="block text-muted text-pretty">{{ lesson.description }}</span>
                </span>
            </NuxtLink>
        </li>
    </ol>
</template>
