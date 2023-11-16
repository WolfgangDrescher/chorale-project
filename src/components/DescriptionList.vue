<script setup>
defineProps({
    items: Array,
});
</script>

<template>
    <dl class="flex flex-wrap gap-4">
        <template v-for="item in items">
            <dt class="flex-grow flex-shrink-0 basis-1/6">
                <slot name="term" :item="item">
                    {{ item.term }}
                </slot>
            </dt>
            <dd class="flex-grow flex-shrink basis-4/6">
                <slot name="detail" :item="item">
                    {{ item.detail }}
                </slot>
            </dd>
        </template>
    </dl>
</template>
