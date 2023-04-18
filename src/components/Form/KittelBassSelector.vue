<script setup>
defineProps(['label', 'modelValue', 'groupLabel']);
const emit = defineEmits(['update:modelValue']);

function onChange(value) {
    emit('update:modelValue', value);
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
                :class="{'bg-primary-500 text-white': modelValue == option}"
                @click="onChange(option)"
            >
                {{ Number.isInteger(option) ? option : $t('allBasses')  }}
            </button>
        </div>
    </FormGroup>
</template>
