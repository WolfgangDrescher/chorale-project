<script setup>
const props = defineProps({
    label: String,
    modelValue: Number,
    groupLabel: String,
    min: {
        type: Number,
        default: 0,
    },
    max: {
        type: Number,
        default: 100,
    },
});
const emit = defineEmits(['update:modelValue']);

const progressBarElem = ref();
const isDragging = ref(false);

function seekHandler(e) {
    const rect = e.target.getBoundingClientRect();
    emit('update:modelValue', getValue((e.clientX - rect.left) / rect.width));
}

function whileMoveMouse(e) {
    e.stopPropagation();
    setValueFromClientX(e.clientX);
    isDragging.value = true;
}

function whileMoveTouch(e) {
    e.stopPropagation();
    setValueFromClientX(e.touches[0].clientX);
    isDragging.value = true;
}

function setValueFromClientX(clientX) {
    const rect = progressBarElem.value.getBoundingClientRect();
    const x = Math.min(Math.max(clientX - rect.left, 0), rect.width);
    emit('update:modelValue', getValue(x / rect.width));
}

function endMove(event) {
    event.stopPropagation();
    window.removeEventListener('mousemove', whileMoveMouse);
    window.removeEventListener('mouseup', endMove, true);
    window.removeEventListener('touchmove', whileMoveTouch);
    window.removeEventListener('touchend', endMove, true);
    isDragging.value = false;
}

function onMousedownEvent(event) {
    event.stopPropagation();
    window.addEventListener('mousemove', whileMoveMouse);
    window.addEventListener('mouseup', endMove, true);
    window.addEventListener('touchmove', whileMoveTouch);
    window.addEventListener('touchend', endMove, true);
    isDragging.value = false;
}

function getValue(value) {
    return (props.max - props.min) / 1 * value + props.min;
}

const progress = computed(() => {
    return 100 / (props.max - props.min) * props.modelValue - (100 / (props.max - props.min) * props.min);
});

function isTouchDevice() {
    return 'ontouchstart' in window || navigator.maxTouchPoints > 0 || navigator.msMaxTouchPoints > 0;
}

const touchDevice = ref(false);
onMounted(() => {
    if (isTouchDevice()) {
        touchDevice.value = true;
    }
});
</script>

<template>
    <FormGroup :label="groupLabel">
        <div
            ref="progressBarElem"
            class="relative flex-grow h-1 bg-gray-200 cursor-pointer rounded"
            :style="`--progress: ${progress}%`"
            @mouseup="seekHandler"
            :class="{'is-dragging': isDragging}"
        >
            <div class="pointer-events-none bg-primary-500 rounded h-full" style="width: var(--progress)"></div>
            <div
                class="group touch-none cursor-grab absolute top-1/2 -translate-y-1/2 -translate-x-1.5 w-3 h-3 rounded-full bg-primary-500 shadow"
                :class="touchDevice ? 'touch-device' : 'no-touch-device'"
                style="left: var(--progress)"
                @mousedown="onMousedownEvent"
                @touchstart="onMousedownEvent"
            >
            <div class="progress-number hidden group-hover:block absolute bg-primary-500 text-white text-xs leading-4 px-1 rounded bottom-full -translate-x-1/2 left-1/2 mb-1">
                {{ Math.round(modelValue) }}
            </div>
        </div>
        </div>
    </FormGroup>
</template>

<style scoped>
.is-dragging .progress-number {
    @apply block;
}
</style>
