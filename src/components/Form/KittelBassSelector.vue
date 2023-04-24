<script setup>
const props = defineProps(['label', 'modelValue', 'groupLabel']);
const emit = defineEmits(['update:modelValue']);

function onChange(value, event) {
    if (value === 'all') {
        emit('update:modelValue', ['all']);
    } else if (Array.isArray(props.modelValue)) {
        if(props.modelValue.includes(value)) {
            const newValue =  props.modelValue.filter(o => o !== value);
            if(newValue.length === 0) {
                newValue.push('all');
            }
            emit('update:modelValue', newValue);
        } else {
            let newValue = [value];
            if (event.altKey || event.ctrlKey || event.metaKey || event.shiftKey) {
                newValue = [...props.modelValue.filter(o => o !== 'all')]
                newValue.push(value);
            }
            emit('update:modelValue', newValue.sort().reverse());
        }
    } else {
        emit('update:modelValue', value);
    }
}

const options = ['all', ...Array.from({ length: 8 }, (_, i) => i + 1)];
</script>

<template>
    <FormGroup :label="groupLabel">
        <div class="flex divide-x divide-primary-500 border border-primary-500 rounded">
            <button
                v-for="option in options"
                :key="option"
                class="grow text-center"
                :class="{'bg-primary-500 text-white': Array.isArray(modelValue) ? modelValue.includes(option) : modelValue == option}"
                @click="onChange(option, $event)"
            >
                {{ Number.isInteger(option) ? option : $t('allBasses')  }}
            </button>
        </div>
    </FormGroup>
</template>
