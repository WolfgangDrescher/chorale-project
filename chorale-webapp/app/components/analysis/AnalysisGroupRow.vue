<script setup>
const props = defineProps({
    group: { type: Object, required: true },
    expandable: { type: Boolean, default: false },
    expanded: { type: Boolean, default: false },
    nested: { type: Boolean, default: false },
    // Whether the label leaves room for the chevron of the rows beside it that open.
    indent: { type: Boolean, default: false },
});

const emit = defineEmits(['toggle', 'query']);

const { t } = useI18n();
const format = inject('analysisFormat');
const showExamples = ref(false);
const showMembers = ref(false);
const label = computed(() => format.label(props.group.label));
const lineColor = computed(() => (props.nested ? 'border-neutral-100 dark:border-neutral-900' : 'border-default'));

const VOICE_NAMES = { 1: 'voiceBass', 2: 'voiceTenor', 3: 'voiceAlto', 4: 'voiceSoprano' };

// "chor001, T. 6–7, ohne Figuration", or the voices that ornament their line there. An example that
// stands for a group below this one is named after it first: "+M2 -P5: chor001, T. 6–7".
function caption(example) {
    const [first, last] = example.measures;
    const measures = first === last ? `${first}` : `${first}–${last}`;
    const ornamentation = example.ornamentedVoices.length
        ? t('analysisOrnamented', { voices: example.ornamentedVoices.map((voice) => t(VOICE_NAMES[voice])).join(', ') })
        : t('analysisUnornamented');
    const text = `${t('analysisExampleCaption', { chorale: example.choraleId, measures })}, ${ornamentation}`;
    return example.groupLabel ? `${format.label(example.groupLabel)}: ${text}` : text;
}
</script>

<template>
    <tr class="border-b last:border-0" :class="lineColor">
        <td class="py-1.5 pr-4">
            <span v-if="group.other" class="inline-flex items-center gap-1.5 text-muted">
                <span v-if="indent" class="size-5 shrink-0" />
                {{ t('analysisOther') }}
            </span>
            <UButton
                v-else-if="expandable"
                :label="label"
                :icon="expanded ? 'lucide:chevron-down' : 'lucide:chevron-right'"
                :aria-label="t('expandGroup', { label: group.label })"
                color="neutral"
                variant="link"
                size="md"
                class="px-0 py-0 font-mono text-default"
                @click="emit('toggle')"
            />
            <UButton
                v-else-if="group.members?.length"
                :label="label"
                :icon="showMembers ? 'lucide:chevron-down' : 'lucide:chevron-right'"
                :aria-label="t('expandGroup', { label: group.label })"
                color="neutral"
                variant="link"
                size="md"
                class="px-0 py-0 font-mono text-default"
                @click="showMembers = !showMembers"
            />
            <span v-else class="inline-flex items-center gap-1.5 font-mono">
                <span v-if="indent" class="size-5 shrink-0" />
                {{ label }}
            </span>
        </td>
        <td class="py-1.5 pr-4 text-right tabular-nums">{{ group.matchCount }}</td>
        <td class="py-1.5 pr-4">
            <div class="flex items-center gap-2">
                <div class="h-2 w-24 rounded-full bg-elevated">
                    <div class="h-2 rounded-full bg-primary" :style="{ width: `${group.share * 100}%` }" />
                </div>
                <span class="tabular-nums">{{ format.percent(group.share) }}</span>
            </div>
        </td>
        <td class="py-1.5 pr-4 text-right tabular-nums">{{ group.choraleCount }}</td>
        <td class="py-1.5 text-right whitespace-nowrap">
            <template v-if="!group.other">
                <UButton
                    v-if="group.examples?.length"
                    icon="lucide:music"
                    :color="showExamples ? 'primary' : 'neutral'"
                    variant="soft"
                    size="xs"
                    class="mr-1"
                    :aria-label="t('showGroupExamples', { label: group.label })"
                    :title="t('showGroupExamples', { label: group.label })"
                    @click="showExamples = !showExamples"
                />
                <UButton
                    v-if="group.query"
                    icon="lucide:search"
                    color="neutral"
                    variant="soft"
                    size="xs"
                    :aria-label="t('showGroupQuery', { label: group.label })"
                    :title="t('showGroupQuery', { label: group.label })"
                    @click="emit('query', group.query)"
                />
                <UButton
                    v-else
                    icon="lucide:circle-slash"
                    color="neutral"
                    variant="link"
                    size="xs"
                    disabled
                    :aria-label="t('analysisNoQuery')"
                    :title="t('analysisNoQuery')"
                />
            </template>
        </td>
    </tr>
    <tr v-if="showExamples" class="border-b" :class="lineColor">
        <td colspan="5" class="pb-3">
            <div class="ml-5 border-l-2 border-accented pl-4">
                <p class="pt-2 text-xs text-muted">{{ t('analysisExamplesOf', { label }) }}</p>
                <div class="w-0 min-w-full px-9">
                    <UCarousel
                        v-slot="{ item: example }"
                        :items="group.examples"
                        :ui="{
                            item: 'basis-56',
                            prev: '-start-9 sm:-start-9',
                            next: '-end-9 sm:-end-9',
                        }"
                        :prev="{ size: 'xs' }"
                        :next="{ size: 'xs' }"
                        :drag-free="false"
                        align="start"
                        contain-scroll="trimSnaps"
                        arrows
                        class="w-full"
                    >
                        <LessonScore
                            :chorale-id="example.choraleId"
                            :lines="`${example.startLine}-${example.endLine}`"
                            :scale="35"
                            :caption="caption(example)"
                        />
                    </UCarousel>
                </div>
            </div>
        </td>
    </tr>
    <tr v-if="showMembers" class="border-b" :class="lineColor">
        <td colspan="5" class="pb-3">
            <div class="ml-5 border-l-2 border-primary/40 pl-4">
                <table class="w-full table-fixed text-sm">
                    <AnalysisColumns />
                    <thead>
                        <tr class="border-b border-default text-left text-xs text-muted">
                            <th class="py-1.5 pr-4 font-medium">{{ t('analysisMember') }}</th>
                            <th class="py-1.5 pr-4 text-right font-medium">{{ t('analysisMatches') }}</th>
                            <th class="py-1.5 pr-4 font-medium">{{ t('analysisShareOf', { label }) }}</th>
                            <th class="py-1.5 pr-4 text-right font-medium">{{ t('analysisChorales') }}</th>
                            <th class="py-1.5 font-medium"><span class="sr-only">{{ t('query') }}</span></th>
                        </tr>
                    </thead>
                    <tbody>
                        <tr
                            v-for="member in group.members"
                            :key="member.label"
                            class="border-b border-neutral-100 last:border-0 dark:border-neutral-900"
                        >
                            <td class="py-1.5 pr-4 font-mono">{{ member.label }}</td>
                            <td class="py-1.5 pr-4 text-right tabular-nums">{{ member.matchCount }}</td>
                            <td class="py-1.5 pr-4">
                                <div class="flex items-center gap-2">
                                    <div class="h-2 w-24 rounded-full bg-elevated">
                                        <div class="h-2 rounded-full bg-primary" :style="{ width: `${(member.matchCount / group.matchCount) * 100}%` }" />
                                    </div>
                                    <span class="tabular-nums">{{ format.percent(member.matchCount / group.matchCount) }}</span>
                                </div>
                            </td>
                            <td class="py-1.5 pr-4 text-right tabular-nums">{{ member.choraleCount }}</td>
                            <td class="py-1.5 text-right whitespace-nowrap">
                                <UButton
                                    v-if="member.query"
                                    icon="lucide:search"
                                    color="neutral"
                                    variant="soft"
                                    size="xs"
                                    :aria-label="t('showGroupQuery', { label: member.label })"
                                    :title="t('showGroupQuery', { label: member.label })"
                                    @click="emit('query', member.query)"
                                />
                            </td>
                        </tr>
                    </tbody>
                </table>
            </div>
        </td>
    </tr>
</template>
