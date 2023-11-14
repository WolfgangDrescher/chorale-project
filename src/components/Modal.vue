<script setup>
defineEmits(['close']);
</script>

<template>
    <ClientOnly>
        <Teleport to="body">
            <div @click="$emit('close')" tabindex="-1" aria-hidden="true" class="overflow-y overflow-x-hidden fixed top-0 right-0 left-0 z-50 justify-center items-center w-full h-screen max-h-full bg-black/80">
                <div class="relative top-1/2 left-1/2 -translate-x-1/2 -translate-y-1/2 w-full max-w-2xl max-h-full" @click.stop>
                    <div class="bg-white rounded shadow">
                        <div class="p-4 space-y-4">
                            <slot></slot>
                        </div>
                    </div>
                </div>
            </div>
        </Teleport>
    </ClientOnly>
</template>
