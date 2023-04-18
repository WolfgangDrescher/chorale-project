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

function seekHandler(e) {
    const rect = e.target.getBoundingClientRect();
    emit('update:modelValue', getValue((e.clientX - rect.left) / rect.width));
}

function whileMoveMouse(e) {
    e.stopPropagation();
    setValueFromClientX(e.clientX);
}

function whileMoveTouch(e) {
    e.stopPropagation();
    setValueFromClientX(e.touches[0].clientX);
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
}

function onMousedownEvent(event) {
    event.stopPropagation();
    window.addEventListener('mousemove', whileMoveMouse);
    window.addEventListener('mouseup', endMove, true);
    window.addEventListener('touchmove', whileMoveTouch);
    window.addEventListener('touchend', endMove, true);
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
            class="relative flex-grow h-1 bg-gray-200 cursor-pointer"
            :style="`--progress: ${progress}%`"
            @mouseup="seekHandler"
        >
            <div class="pointer-events-none bg-primary-500 h-full" style="width: var(--progress)"></div>
            <div
                class="cursor-grab absolute top-1/2 -translate-y-1/2 -translate-x-1.5 w-3 h-3 rounded-full bg-primary-500 shadow"
                :class="touchDevice ? 'touch-device' : 'no-touch-device'"
                style="left: var(--progress)"
                @mousedown="onMousedownEvent"
                @touchstart="onMousedownEvent"
            ></div>
        </div>
    </FormGroup>
</template>
