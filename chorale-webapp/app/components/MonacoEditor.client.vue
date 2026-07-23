<script setup>
import * as monaco from 'monaco-editor';
import { jsonDefaults } from 'monaco-editor/language/json/monaco.contribution';

const modelValue = defineModel();

const props = defineProps({
    options: {
        type: Object,
        default: () => ({}),
    },
    schema: {
        type: Object,
        default: null,
    },
});

const monacoEl = ref();
let editor = null;

watch(modelValue, (newValue) => {
    if (!editor) return;
    if (newValue !== editor.getValue()) {
        editor.setValue(newValue ?? '');
    }
});

onMounted(() => {
    nextTick(() => {
        if (props.schema) {
            jsonDefaults.setDiagnosticsOptions({
                validate: true,
                schemas: [
                    {
                        uri: 'inmemory://schema/chorale-search-query.json',
                        fileMatch: ['*'],
                        schema: props.schema,
                    },
                ],
            });
        }
        editor = monaco.editor.create(monacoEl.value, Object.assign({
            value: modelValue.value,
            language: 'json',
            scrollBeyondLastLine: false,
            // wordWrap: 'on',
            wrappingStrategy: 'advanced',
            minimap: {
                enabled: false,
            },
            stickyScroll: {
                enabled: false,
            },
            fontSize: 16,
            // readOnly: true,
            hideCursorInOverviewRuler: true,
            overviewRulerLanes: 0,
            theme: 'vs-dark',
        }, props.options));
        editor.getModel().onDidChangeContent(() => modelValue.value = editor.getValue());

        // The suggestion details panel (property/enum descriptions) is collapsed by
        // default and its open/closed state is a flag shared across every editor on the
        // page, persisted only for this page load. Flipping it the first time the
        // suggest widget appears makes it show expanded from then on, without a manual
        // toggle click every time.
        const suggestController = editor.getContribution('editor.contrib.suggestController');
        if (suggestController) {
            const widget = suggestController.widget.value;
            const subscription = widget.onDidShow(() => {
                editor.trigger('chorale-webapp', 'toggleSuggestionDetails', {});
                subscription.dispose();
            });
        }
    });
});
</script>

<template>
    <div ref="monacoEl" class="w-full h-full min-h-75"></div>
</template>
