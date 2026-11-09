<script setup>
// The entries of a useModulationAnnotator as chips, the state of the last save and the keys. A
// click on a chip hands the focus back, so the keys keep working.
const props = defineProps({
    annotator: { type: Object, required: true },
});

// The field of the key, in the chip it is open in: a list, as refs in a v-for are.
const inputs = useTemplateRef('input');

// Modifiers are alternatives to each other, the keys are pressed with one of them.
const KEY_GROUPS = [
    [
        { keys: ['space'], action: 'annotatorPlay' },
        { keys: ['arrowleft', 'arrowright'], action: 'annotatorMoveFrame' },
        { modifiers: ['alt'], keys: ['arrowleft', 'arrowright'], action: 'annotatorJumpByPhrase' },
        { modifiers: ['meta'], keys: ['arrowleft', 'arrowright'], action: 'annotatorJumpToEitherEnd' },
    ],
    [
        { keys: ['enter'], action: 'annotatorMark' },
        { keys: ['G', 'e', 'B-'], action: 'annotatorSetKey' },
        { keys: ['?'], action: 'annotatorLeaveKeyOpen' },
        { modifiers: ['shift'], keys: ['arrowleft', 'arrowright'], action: 'annotatorMoveEntry' },
        { keys: ['backspace'], action: 'annotatorDeleteEntry' },
        { keys: ['tab'], action: 'annotatorNextEntry' },
        { modifiers: ['shift'], keys: ['tab'], action: 'annotatorPreviousEntry' },
    ],
];

// The field of the key is focused when it opens, and the chip it is in scrolled into view.
watch(() => props.annotator.input === null, async (closed) => {
    if (closed) return;
    await nextTick();
    const input = [inputs.value].flat()[0]?.inputRef;
    input?.focus();
    input?.scrollIntoView({ block: 'nearest', inline: 'nearest' });
});
</script>

<template>
    <div class="flex flex-col gap-1">
        <div class="flex items-center gap-3 text-xs">
            <div class="flex-1 min-w-0 flex items-center gap-1 overflow-x-auto py-0.5 [scrollbar-width:none]">
                <UTooltip
                    v-for="entry in annotator.entries"
                    :key="entry.line"
                    :text="$t('modulationPositionAmbiguous', { position: annotator.positionOf(entry.line) })"
                    :disabled="!annotator.isAmbiguous(entry.line)"
                >
                    <UFieldGroup size="xs" class="shrink-0">
                        <UButton
                            color="neutral"
                            :variant="entry.line === annotator.line ? 'solid' : 'subtle'"
                            :icon="annotator.isAmbiguous(entry.line) ? 'lucide:triangle-alert' : undefined"
                            class="tabular-nums"
                            @click="annotator.goTo(entry); $event.currentTarget.blur()"
                        >
                            {{ annotator.positionOf(entry.line) }}
                        </UButton>
                        <UInput
                            v-if="entry.line === annotator.inputLine"
                            ref="input"
                            v-model="annotator.input"
                            class="w-12"
                            :placeholder="$t('key')"
                            @keydown.enter.prevent="annotator.commitInput()"
                            @keydown.space.prevent.stop="annotator.commitInput(); annotator.resume()"
                            @keydown.esc.prevent="annotator.closeInput()"
                            @blur="annotator.commitInput()"
                        />
                        <UButton
                            v-else
                            :color="entry.key === '?' ? 'warning' : 'success'"
                            :variant="entry.line === annotator.line ? 'solid' : 'subtle'"
                            class="font-semibold min-w-7 justify-center"
                            @click="annotator.goTo(entry); annotator.openInput(entry.key === '?' ? '' : entry.key, entry.line); $event.currentTarget.blur()"
                        >
                            {{ entry.key }}
                        </UButton>
                    </UFieldGroup>
                </UTooltip>
            </div>
            <div class="flex shrink-0 items-center gap-1">
                <UBadge v-if="annotator.status === 'saving'" color="neutral" variant="soft" icon="lucide:loader-circle" :ui="{ leadingIcon: 'animate-spin' }" :label="$t('modulationsSaving')" />
                <UBadge v-else-if="annotator.status === 'saved'" color="success" variant="soft" icon="lucide:check" :label="$t('modulationsSaved')" />
                <UTooltip v-else-if="annotator.status === 'failed'" :text="annotator.error">
                    <UBadge color="error" variant="soft" icon="lucide:circle-alert" :label="$t('modulationsSaveFailed')" />
                </UTooltip>
                <UTooltip v-if="annotator.warnings.length" :text="annotator.warnings.join(' · ')">
                    <UBadge color="warning" variant="soft" icon="lucide:triangle-alert" :label="annotator.warnings.length" />
                </UTooltip>
                <!-- On hover and without focus, so the keys keep working. A click on the button
                     counts as outside of the popover and would close it. -->
                <UPopover mode="hover" :content="{ align: 'end', onPointerDownOutside: (event) => event.preventDefault() }">
                    <UButton size="xs" color="neutral" variant="ghost" icon="lucide:keyboard" :aria-label="$t('annotatorKeys')" @mousedown.prevent />
                    <template #content>
                        <div class="w-80 py-1.5 text-sm">
                            <template v-for="(group, groupIndex) in KEY_GROUPS" :key="groupIndex">
                                <div v-if="groupIndex" class="my-1.5 border-t border-default" />
                                <div v-for="hint in group" :key="hint.action" class="flex items-center justify-between gap-4 px-3 py-1">
                                    <span>{{ $t(hint.action) }}</span>
                                    <span class="flex items-center gap-0.5 text-muted">
                                        <template v-for="(modifier, index) in hint.modifiers" :key="modifier">
                                            <span v-if="index">/</span>
                                            <UKbd :value="modifier" variant="soft" />
                                        </template>
                                        <UKbd v-for="key in hint.keys" :key="key" :value="key === 'space' ? $t('annotatorSpace') : key" variant="soft" class="normal-case" />
                                    </span>
                                </div>
                            </template>
                            <p class="mt-1.5 border-t border-default px-3 pt-2 pb-1 text-xs text-muted">{{ $t('annotatorKeyNames') }}</p>
                        </div>
                    </template>
                </UPopover>
                <UTooltip :text="$t('downloadModulations')">
                    <UButton size="xs" color="neutral" variant="ghost" icon="lucide:download" :aria-label="$t('downloadModulations')" @click="annotator.download()" />
                </UTooltip>
                <UTooltip :text="$t('createGithubIssue')">
                    <UButton size="xs" color="neutral" variant="ghost" icon="lucide:github" :aria-label="$t('createGithubIssue')" :to="annotator.issueUrl" target="_blank" />
                </UTooltip>
            </div>
        </div>
    </div>
</template>
