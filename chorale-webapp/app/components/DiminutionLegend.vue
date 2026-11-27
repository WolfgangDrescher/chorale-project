<script setup>
// The diminutions of the analysis layer of a chorale, as returned by the legend of
// useDiminutionAnalysis: color, name and number of figures, the uncertain ones apart, each shown or hidden
// with its checkbox.
// With an annotator, they can be shown as annotated by hand instead and annotated. The controls have a
// width of their own, so annotating does not change where the labels break.
const props = defineProps({
    legend: { type: Array, required: true },
    // See useDiminutionAnnotator.
    annotator: { type: Object, required: true },
    // Whether the chorale has annotations to show.
    annotated: { type: Boolean, default: false },
});

const emit = defineEmits(['toggle', 'annotateFound']);

// "found" or "annotated".
const source = defineModel('source', { type: String, default: 'found' });
const annotating = defineModel('annotating', { type: Boolean, default: false });

const { t } = useI18n();
const { macOS } = useKbd();
const altKeyName = computed(() => (macOS.value ? 'Option' : 'Alt'));

// Annotating shows what is annotated, so that it is clear what a click changes.
const sourceItems = computed(() => [
    { label: t('diminutionsFound'), value: 'found', disabled: annotating.value },
    { label: t('diminutionsAnnotated'), value: 'annotated' },
]);

// What replaces all annotations is asked about first, in place of its buttons.
const replacingActions = computed(() => [
    { action: 'annotateFound', icon: 'lucide:import', label: t('annotateFoundDiminutions'), confirmation: t('annotateFoundDiminutionsConfirmation'), confirmAction: t('annotateFoundDiminutionsAction'), run: () => emit('annotateFound') },
    { action: 'clear', icon: 'lucide:eraser', label: t('clearDiminutionAnnotations'), confirmation: t('clearDiminutionAnnotationsConfirmation'), confirmAction: t('clearDiminutionAnnotationsAction'), run: () => props.annotator.clear() },
]);
const confirming = ref(null);

// Only a diminution that is shown can be annotated, so choosing a hidden one shows it.
const isChosen = (diminution) => annotating.value && diminution.shown && props.annotator.current === diminution.id;

function choose(diminution) {
    if (!diminution.shown) emit('toggle', diminution.id);
    props.annotator.current = diminution.id;
}

watch(annotating, () => {
    confirming.value = null;
});

function confirm() {
    confirming.value.run();
    confirming.value = null;
}

// The state of the last save in place of the pencil, so the button keeps its size.
const annotateIcon = computed(() => ({
    saving: 'lucide:loader-circle',
    saved: 'lucide:check',
    failed: 'lucide:circle-alert',
})[annotating.value && props.annotator.status] ?? 'lucide:pencil');
</script>

<template>
    <div class="flex flex-col gap-3">
        <div class="flex flex-wrap items-start gap-x-6 gap-y-3">
            <ul class="flex flex-1 flex-wrap gap-x-5 gap-y-2 text-sm">
                <li v-for="diminution in legend" :key="diminution.id">
                    <UCheckbox :model-value="diminution.shown" @update:model-value="emit('toggle', diminution.id)">
                        <template #label>
                            <span class="flex items-center gap-1.5">
                                <button
                                    type="button"
                                    class="size-4 shrink-0 rounded-full"
                                    :class="annotating ? 'cursor-pointer' : 'pointer-events-none'"
                                    :style="{
                                        backgroundColor: highlightColorsByName[diminution.color],
                                        boxShadow: isChosen(diminution) ? '0 0 0 2px var(--ui-bg), 0 0 0 4px var(--ui-primary)' : undefined,
                                    }"
                                    :tabindex="annotating ? 0 : -1"
                                    :aria-label="$t('annotateDiminution', { name: $t(`diminution.${diminution.id}`) })"
                                    :aria-pressed="isChosen(diminution)"
                                    @click.stop.prevent="choose(diminution)"
                                />
                                <span>{{ $t(`diminution.${diminution.id}`) }}</span>
                                <span class="min-w-[2ch] text-dimmed tabular-nums">{{ diminution.count }}<template v-if="diminution.uncertainCount"> + {{ diminution.uncertainCount }}?</template></span>
                            </span>
                        </template>
                    </UCheckbox>
                </li>
            </ul>
            <div class="flex w-full shrink-0 items-center justify-end gap-2 sm:w-100">
                <UTabs
                    v-if="annotated || annotating"
                    :model-value="annotating ? 'annotated' : source"
                    :items="sourceItems"
                    :content="false"
                    size="xs"
                    @update:model-value="source = $event"
                />
                <UTooltip :text="annotator.error ?? undefined" :disabled="!annotating || annotator.status !== 'failed'">
                    <UButton
                        size="xs"
                        color="neutral"
                        :variant="annotating ? 'soft' : 'ghost'"
                        :icon="annotateIcon"
                        :ui="{ leadingIcon: annotator.status === 'saving' && annotating ? 'animate-spin' : '' }"
                        :label="$t(annotating ? 'annotateDone' : 'annotateDiminutions')"
                        :aria-pressed="annotating"
                        @click="annotating = !annotating"
                    />
                </UTooltip>
                <template v-if="annotating">
                    <UTooltip :text="$t('downloadModulations')">
                        <UButton size="xs" color="neutral" variant="ghost" icon="lucide:download" :aria-label="$t('downloadModulations')" @click="annotator.download()" />
                    </UTooltip>
                    <UTooltip :text="$t('createGithubIssue')">
                        <UButton size="xs" color="neutral" variant="ghost" icon="lucide:github" :aria-label="$t('createGithubIssue')" :to="annotator.issueUrl" target="_blank" />
                    </UTooltip>
                </template>
            </div>
        </div>
        <div v-if="annotating" class="flex flex-wrap items-start justify-between gap-x-6 gap-y-2">
            <p class="max-w-prose text-xs text-muted">{{ $t('annotateDiminutionsHint', { alt: altKeyName }) }}</p>
            <ConfirmationPrompt
                v-if="confirming"
                :question="confirming.confirmation"
                :action="confirming.confirmAction"
                @confirm="confirm"
                @cancel="confirming = null"
            />
            <div v-else class="flex shrink-0 gap-1">
                <UButton
                    v-for="replacing in replacingActions"
                    :key="replacing.action"
                    size="xs"
                    color="neutral"
                    variant="ghost"
                    :icon="replacing.icon"
                    :label="replacing.label"
                    @click="confirming = replacing"
                />
            </div>
        </div>
    </div>
</template>
