<script setup>
import * as monaco from 'monaco-editor';

const modelValue = defineModel();

const props = defineProps({
    options: {
        type: Object,
        default: () => ({}),
    },
});

const monacoEl = ref();
let editor = null;

onMounted(() => {
    nextTick(() => {
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
    });
});
</script>

<template>
    <div ref="monacoEl" class="w-full h-full min-h-75"></div>
</template>
