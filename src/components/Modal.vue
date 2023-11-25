<script setup>
import { onKeyStroke } from '@vueuse/core';

defineProps(['title']);
const emit = defineEmits(['close']);

onKeyStroke('Escape', () => {
   emit('close');
});

onMounted(() => {
    document.body.classList.add('modal-is-open');
});

onUnmounted(() => {
    document.body.classList.remove('modal-is-open');
});
</script>

<template>
    <ClientOnly>
        <Teleport to="body">
            <div @click="$emit('close')" tabindex="-1" aria-hidden="true" class="overflow-y overflow-x-hidden fixed top-0 right-0 left-0 z-50 justify-center items-center w-full h-screen max-h-full bg-black/80">
                <div class="relative top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 w-full max-w-2xl max-h-full" @click.stop>
                    <div class="p-4">
                        <div class="bg-white rounded shadow">
                            <div class="border-b px-4 py-2 text-lg font-bold">
                                <slot name="title">
                                    {{ title }}
                                </slot>
                            </div>
                            <div class="p-4 space-y-4">
                                <slot></slot>
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </Teleport>
    </ClientOnly>
</template>

<style>
body.modal-is-open {
    overflow: hidden;
}
</style>
