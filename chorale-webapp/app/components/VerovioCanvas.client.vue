<script setup>
import 'vue-verovio-canvas/style.css';
import { VerovioCanvas, createWorkerVerovioToolkit } from 'vue-verovio-canvas';

const { verovioWorker } = useVerovioWorker();
const toolkit = ref();

const emit = defineEmits(['scoreIsReady', 'scoreLoaded']);

function loadToolkit() {
    toolkit.value = createWorkerVerovioToolkit(verovioWorker);
}

function unloadToolkit() {
    if (toolkit.value) {
        toolkit.value.removeToolkit();
        toolkit.value = null;
    }
}

function scoreIsReady(event) {
    emit('scoreIsReady', event);
}

// Unlike scoreIsReady, also when the score is loaded again, e.g. with other filters.
function scoreLoaded(event) {
    emit('scoreLoaded', event);
}
</script>

<template>
    <VerovioCanvas ref="verovioCanvas" lazy :toolkit="toolkit" @load="loadToolkit" @unload="unloadToolkit" @score-is-ready="scoreIsReady" @score-loaded="scoreLoaded" />
</template>
