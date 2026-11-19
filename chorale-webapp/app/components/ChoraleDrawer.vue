<script setup>
const props = defineProps({
    choraleId: { type: String, required: true },
});

const open = defineModel('open', { type: Boolean, default: false });

const { t } = useI18n();
const localePath = useLocalePath();

const { data: chorale } = await useAsyncData(`bach-chorales/${props.choraleId}`, () => {
    return queryCollection('bachChorales').where('stem', '=', `bach-chorales/${props.choraleId}`).first();
});

const title = computed(() => [chorale.value?.number, chorale.value?.title ?? props.choraleId].filter(Boolean).join('. '));
const description = computed(() => [props.choraleId, chorale.value?.bwv].filter(Boolean).join(' · '));

// As in QueryDrawer: the drawer takes the focus, so the space bar plays the score.
const content = {
    onOpenAutoFocus: (event) => {
        event.preventDefault();
        event.target.focus();
    },
    onCloseAutoFocus: (event) => event.preventDefault(),
};
</script>

<template>
    <UDrawer
        v-model:open="open"
        direction="right"
        :handle="false"
        handle-only
        close
        :title="title"
        :description="description"
        :content="content"
        :ui="{ content: 'w-full max-w-5xl select-text!', container: 'min-w-0 overflow-x-hidden' }"
    >
        <template #actions>
            <UButton
                color="neutral"
                variant="ghost"
                size="sm"
                icon="lucide:external-link"
                :label="t('openChoralePage')"
                :to="localePath({ name: 'chorale-bach-id', params: { id: choraleId } })"
                @click="open = false"
            />
        </template>
        <template #body>
            <ChoraleView v-if="chorale" :chorale="chorale" />
            <UAlert v-else :title="t('referenceMissing', { id: choraleId })" color="warning" variant="subtle" icon="lucide:unlink" />
        </template>
    </UDrawer>
</template>
