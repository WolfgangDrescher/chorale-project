<script setup>
const props = defineProps({
    url: {
        type: String,
        required: true,
    },
    verovioOptions: {
        type: Object,
        default: () => ({}),
    },
    scoreFormatter: {
        type: Object,
        required: true,
    },
});

const emit = defineEmits([
    'scoreIsReady',
]);

const verovioCanvas = ref(null);
const { setData, formattedScoreData } = props.scoreFormatter;

const response = await fetch(props.url);
if (!response.ok) {
    throw createError({ statusCode: response.status, statusMessage: response.statusText });
}
setData(await response.text())

const scale = ref(40);
const verovioCanvasOptions = computed(() => {
    return Object.assign({
        pageMargin: 50,
        options: {
            ...props.verovioOptions,
            svgBoundingBoxes: true,
            svgViewBox: true,
        },
        scale: scale.value,
        data: formattedScoreData.value,
    });
});

let callVerovioMethod;

function verovioCanvasScoreIsReady(event) {
    callVerovioMethod = event.callVerovioMethod;
    emit('scoreIsReady', event);
}

defineExpose({
    verovioCanvas,
});
</script>

<template>
    <div class="flex flex-col gap-4">
        <div>
            <div class="mt-4 flex flex-col gap-4">
                <slot></slot>
            </div>
        </div>
        <div>
            <div class="grid md:grid-cols-3 gap-4 my-4">
                <div>
                    <FormSlider v-model="scale" :min="20" :max="60" :group-label="$t('zoom')"></FormSlider>
                </div>
            </div>
            <VerovioCanvas ref="verovioCanvas" v-bind="verovioCanvasOptions" @score-is-ready="verovioCanvasScoreIsReady" />
        </div>
    </div>
</template>
