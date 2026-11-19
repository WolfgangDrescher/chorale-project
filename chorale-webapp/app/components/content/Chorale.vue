<script setup>
import { useClipboard } from '@vueuse/core';

// A Bach chorale named by its number and title in the text, `:chorale{id="chor001"}`. It opens in a drawer.
const props = defineProps({
    id: { type: String, required: true },
});

const { t } = useI18n();
const localePath = useLocalePath();
const { copy, copied } = useClipboard();
const { data: chorale } = await useAsyncData(`chorale-ref-${props.id}`, () => {
    return queryCollection('bachChorales').where('stem', '=', `bach-chorales/${props.id}`).select('number', 'title', 'bwv', 'key', 'mode', 'modulations', 'cantusFirmus').first();
});
const startKey = computed(() => chorale.value && startKeyOf(chorale.value));
const label = computed(() => {
    if (!chorale.value?.title) return props.id;
    return t('quoted', { text: [chorale.value.number && `${chorale.value.number}.`, chorale.value.title].filter(Boolean).join(' ') });
});

const drawerOpen = ref(false);
const drawerMounted = ref(false);

// A click with a modifier key opens the page of the chorale as usual.
function openDrawer(event) {
    if (event.button !== 0 || event.metaKey || event.ctrlKey || event.shiftKey || event.altKey) return;
    event.preventDefault();
    drawerMounted.value = true;
    drawerOpen.value = true;
}
</script>

<template>
    <UPopover mode="hover" arrow :content="{ side: 'top' }" :disabled="!chorale">
        <a :href="localePath({ name: 'chorale-bach-id', params: { id } })" class="text-primary transition-colors hover:text-primary/75" @click="openDrawer">{{ label }}</a>
        <template #content>
            <span class="flex items-center gap-2 px-3 py-1.5 text-sm">
                <UButton
                    :label="id"
                    :trailing-icon="copied ? 'lucide:copy-check' : 'lucide:copy'"
                    :title="copied ? t('copied') : t('copy')"
                    color="neutral"
                    variant="soft"
                    size="xs"
                    class="font-mono"
                    :ui="{ trailingIcon: 'text-dimmed' }"
                    @click="copy(id)"
                />
                <span v-if="chorale.bwv">{{ chorale.bwv }}</span>
                <ChoraleKeyBadge v-if="startKey" :key-context="startKey" :cantus-firmus="differingCantusFirmusKeyOf(chorale)" />
            </span>
        </template>
    </UPopover>
    <ChoraleDrawer v-if="drawerMounted" v-model:open="drawerOpen" :chorale-id="id" />
</template>
