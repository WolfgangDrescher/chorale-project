<script setup>
const props = defineProps({
    label: String,
    modelValue: Array,
    groupLabel: String,
    labelFormatter: Function,
    min: {
        type: Number,
        default: 0,
    },
    max: {
        type: Number,
        default: 1,
    },
    step: {
        type: Number,
        default: 1,
    },
});
const emit = defineEmits(['update:modelValue']);
const draggedIndex = ref();

const progressBarElem = ref();
const isDragging = ref(false);
const tmpValue = ref(props.modelValue);

watch(() => props.modelValue, (value) => {
    tmpValue.value = toRaw(value)
});

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
    tmpValue.value = buildModelValue(getValue(x / rect.width));
}

function buildModelValue(value) {
    if (draggedIndex.value === 0) {
        return [value, tmpValue.value[1]]
    }
    return [tmpValue.value[0], value];
}

function endMove(event) {
    event.stopPropagation();
    window.removeEventListener('mousemove', whileMoveMouse);
    window.removeEventListener('mouseup', endMove, true);
    window.removeEventListener('touchmove', whileMoveTouch);
    window.removeEventListener('touchend', endMove, true);
    isDragging.value = false;
    emit('update:modelValue', tmpValue.value);
}

function onMousedownEvent(event, index) {
    event.stopPropagation();
    draggedIndex.value = index;
    window.addEventListener('mousemove', whileMoveMouse);
    window.addEventListener('mouseup', endMove, true);
    window.addEventListener('touchmove', whileMoveTouch);
    window.addEventListener('touchend', endMove, true);
    isDragging.value = false;
}

function getValue(value) {
    return Math.round(Math.pow(props.step, -1) * ((props.max - props.min) / 1 * value + props.min)) / Math.pow(props.step, -1);
}

const progressLeft = computed(() => {
    return Math.max(0, 100 / (props.max - props.min) * tmpValue.value[0] - (100 / (props.max - props.min) * props.min));
});

const progressRight = computed(() => {
    return Math.min(100, 100 / (props.max - props.min) * tmpValue.value[1] - (100 / (props.max - props.min) * props.min));
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

const formattedLabel = computed(() => {
    return (value) => {
        if (props.labelFormatter && typeof props.labelFormatter === 'function') {
            return props.labelFormatter(value);
        }
        return value;
    };
});
</script>

<template>
    <FormGroup :label="groupLabel">
        <div
            ref="progressBarElem"
            class="relative flex-grow h-1 bg-gray-200 cursor-pointer rounded"
            :style="`--progressLeft: ${progressLeft}%; --progressRight: ${progressRight}%`"
        >
            <div class="pointer-events-none bg-primary-500 rounded h-full absolute top-0" style="width: calc(var(--progressRight) - var(--progressLeft)); left: var(--progressLeft);"></div>
            <div
                class="group/left touch-none cursor-grab absolute top-1/2 -translate-y-1/2 -translate-x-1.5 w-3 h-3 rounded-full bg-primary-500 shadow"
                :class="[
                    touchDevice ? 'touch-device' : 'no-touch-device',
                    { 'is-dragging': isDragging && draggedIndex === 0}
                ]"
                style="left: var(--progressLeft)"
                @mousedown="onMousedownEvent($event, 0)"
                @touchstart="onMousedownEvent($event, 0)"
            >
                <div class="progress-number hidden whitespace-nowrap group-hover/left:block absolute bg-primary-500 text-white text-xs leading-4 px-1 rounded bottom-full -translate-x-1/2 left-1/2 mb-1">
                    {{ formattedLabel(tmpValue[0]) }}
                </div>
            </div>

            <div
                class="group/right touch-none cursor-grab absolute top-1/2 -translate-y-1/2 -translate-x-1.5 w-3 h-3 rounded-full bg-primary-500 shadow"
                :class="[
                    touchDevice ? 'touch-device' : 'no-touch-device',
                    { 'is-dragging': isDragging && draggedIndex === 1 }
                ]"
                style="left: var(--progressRight)"
                @mousedown="onMousedownEvent($event, 1)"
                @touchstart="onMousedownEvent($event, 1)"
            >
                <div class="progress-number hidden whitespace-nowrap group-hover/right:block absolute bg-primary-500 text-white text-xs leading-4 px-1 rounded bottom-full -translate-x-1/2 left-1/2 mb-1">
                    {{ formattedLabel(tmpValue[1]) }}
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
