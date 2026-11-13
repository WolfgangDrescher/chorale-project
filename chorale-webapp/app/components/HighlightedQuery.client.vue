<script setup>
import * as monaco from 'monaco-editor';
import { useClipboard } from '@vueuse/core';
import { useLocale } from '@nuxt/ui/composables/useLocale';

// A read-only query in the colors of the editor, with a copy button as on the code blocks of the docs.
const props = defineProps({
    query: { type: String, required: true },
});

const { t: tUi } = useLocale();
const appConfig = useAppConfig();
const { copy, copied } = useClipboard();
const rootRef = useTemplateRef('rootRef');
const tooltipProps = useCodeBlockTooltipProps(rootRef);

// colorize uses the theme the editor has set, vs-dark.
const html = ref('');
watch(() => props.query, async (query) => {
    html.value = await monaco.editor.colorize(query, 'json', { tabSize: 4 });
}, { immediate: true });
</script>

<template>
    <div ref="rootRef" class="relative group">
        <UTooltip v-bind="tooltipProps" :text="tUi('prose.pre.copy')">
            <UButton
                :icon="copied ? appConfig.ui.icons.copyCheck : appConfig.ui.icons.copy"
                color="neutral"
                variant="outline"
                size="sm"
                :aria-label="tUi('prose.pre.copy')"
                class="absolute top-[11px] end-[11px] lg:opacity-0 lg:group-hover:opacity-100 lg:focus-visible:opacity-100 transition"
                @click="copy(query)"
            />
        </UTooltip>
        <pre class="text-xs overflow-auto max-h-[50vh] rounded-md bg-[#1e1e1e] text-[#d4d4d4] p-4" v-html="html" />
    </div>
</template>
