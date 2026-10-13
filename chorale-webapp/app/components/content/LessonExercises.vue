<script setup>
const { t } = useI18n();
const localePath = useLocalePath();
const lesson = inject('lesson', null);

const FORMAT_ICONS = { pdf: 'lucide:file-text', mei: 'lucide:file-music' };

function exportLink(chorale, { format, content = 'cantus-firmus', layout = 'piano', voices = '4', orientation = 'portrait' }) {
    const query = new URLSearchParams({ format, content, layout });
    if (format === 'mei') query.set('voices', voices);
    else query.set('orientation', orientation);
    return {
        to: `/api/chorales/${chorale}/export?${query}`,
        download: `${chorale}-${content}-${layout}-${format === 'mei' ? voices : orientation}.${format}`,
        external: true,
    };
}

const TYPE_ICONS = { composition: 'lucide:pencil-line', analysis: 'lucide:microscope', comparison: 'lucide:git-compare-arrows' };

const items = computed(() => (lesson?.exercises ?? []).map((exercise, index) => ({
    date: t('exerciseNumber', { number: index + 1 }),
    title: exercise.title,
    description: exercise.description,
    icon: TYPE_ICONS[exercise.type ?? 'composition'],
    chorales: exercise.chorales.map((chorale) => ({
        label: chorale,
        score: exercise.scoreFilters && {
            to: { path: localePath(`/chorale/${chorale}`), query: { filters: exercise.scoreFilters.join(',') } },
            ariaLabel: t('exerciseOpenScore', { chorale }),
        },
        entries: (exercise.export?.formats ?? ['pdf', 'mei']).map((format) => ({
            label: format.toUpperCase(),
            icon: FORMAT_ICONS[format],
            ...exportLink(chorale, { ...exercise.export, format }),
        })),
    })),
})));
</script>

<template>
    <UTimeline
        :items="items"
        :default-value="items.length - 1"
        :ui="{
            date: 'text-default text-sm font-medium',
            title: 'text-highlighted text-base font-semibold',
            description: 'text-default text-base',
        }"
        class="not-prose"
    >
        <template #description="{ item }">
            <p class="text-pretty">{{ item.description }}</p>
            <div class="mt-3 flex flex-wrap gap-2">
                <template v-for="chorale in item.chorales" :key="chorale.label">
                    <UDropdownMenu v-if="chorale.entries.length > 1" :items="chorale.entries">
                        <UButton
                            :label="chorale.label"
                            icon="lucide:download"
                            trailing-icon="lucide:chevron-down"
                            color="neutral"
                            variant="outline"
                            size="sm"
                        />
                    </UDropdownMenu>
                    <UButton
                        v-else
                        v-bind="chorale.entries[0]"
                        :label="chorale.label"
                        icon="lucide:download"
                        color="neutral"
                        variant="outline"
                        size="sm"
                    />
                    <UButton
                        v-if="chorale.score"
                        :to="chorale.score.to"
                        :label="chorale.label"
                        :aria-label="chorale.score.ariaLabel"
                        icon="lucide:eye"
                        color="neutral"
                        variant="outline"
                        size="sm"
                    />
                </template>
            </div>
        </template>
    </UTimeline>
</template>
