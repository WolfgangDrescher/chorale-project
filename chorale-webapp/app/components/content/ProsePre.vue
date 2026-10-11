<script>
import theme from '#build/ui/prose/pre';
</script>

<script setup>
// Nuxt UI's code block, plus a button next to the copy button that runs the block's query and
// shows the matches in a drawer, for blocks that hold a complete query.
import { useClipboard } from '@vueuse/core';
import UCodeIcon from '@nuxt/ui/components/prose/CodeIcon.vue';
import { tv } from '@nuxt/ui/runtime/utils/tv';
import { useLocale } from '@nuxt/ui/composables/useLocale';

const props = defineProps({
    icon: { type: null, required: false },
    code: { type: String, required: false },
    language: { type: String, required: false },
    filename: { type: String, required: false },
    highlights: { type: Array, required: false },
    hideHeader: { type: Boolean, required: false },
    meta: { type: String, required: false },
    class: { type: null, required: false },
    ui: { type: Object, required: false },
});

const { t } = useI18n();
const { t: tUi } = useLocale();
const { copy: copyToClipboard, copied } = useClipboard();
const appConfig = useAppConfig();
const baseRef = useTemplateRef('baseRef');
const ui = computed(() => tv({ extend: theme, ...appConfig.ui?.prose?.pre || {} })());

function copyCode() {
    copyToClipboard(props.code ?? baseRef.value?.textContent ?? '');
}

// A complete query has a feature and a pattern. Snippets of a pattern, of a single step or of
// options alone do not, and are not run.
function isCompleteQuery(query) {
    return query !== null && typeof query === 'object' && 'feature' in query && 'pattern' in query;
}

const isRunnable = computed(() => {
    if (props.language !== 'json' || !props.code) return false;
    try {
        const query = JSON.parse(props.code);
        return Array.isArray(query) ? query.length > 0 && query.every(isCompleteQuery) : isCompleteQuery(query);
    } catch {
        return false;
    }
});

// The code of a block ends with a line break, which the editor would show as an empty last line.
const query = computed(() => props.code?.trimEnd());
const search = useChoraleSearch(query.value);
const drawerOpen = ref(false);

// Opening the drawer starts from the query of the code block again, and searches unless the
// results shown are already those of that query.
watch(drawerOpen, (open) => {
    if (!open || search.pending.value) return;
    search.query.value = query.value;
    if (search.searchedQuery.value !== search.query.value) {
        search.fetchSearchResults();
    }
});
</script>

<template>
    <div :class="ui.root({ class: [props.ui?.root], filename: !!props.filename })">
        <div v-if="props.filename && !props.hideHeader" :class="ui.header({ class: props.ui?.header })">
            <UCodeIcon :icon="props.icon" :filename="props.filename" :class="ui.icon({ class: props.ui?.icon })" />

            <span :class="ui.filename({ class: props.ui?.filename })">{{ props.filename }}</span>
        </div>

        <UButton
            v-if="isRunnable"
            icon="lucide:play"
            color="neutral"
            variant="outline"
            size="sm"
            :aria-label="t('showQueryResults')"
            :class="ui.copy({ class: ['end-[51px]', props.ui?.copy] })"
            @click="drawerOpen = true"
        />

        <UButton
            :icon="copied ? appConfig.ui.icons.copyCheck : appConfig.ui.icons.copy"
            color="neutral"
            variant="outline"
            size="sm"
            :aria-label="tUi('prose.pre.copy')"
            :class="ui.copy({ class: props.ui?.copy })"
            @click="copyCode"
        />

        <pre ref="baseRef" :class="ui.base({ class: [props.ui?.base, props.class] })" v-bind="$attrs"><slot /></pre>

        <UDrawer
            v-if="isRunnable"
            v-model:open="drawerOpen"
            direction="right"
            :handle="false"
            handle-only
            close
            :title="t('searchResults')"
            :ui="{ content: 'w-full max-w-4xl select-text!', container: 'min-w-0 overflow-x-hidden' }"
        >
            <template #body>
                <UCollapsible class="mb-4">
                    <UButton
                        color="neutral"
                        variant="link"
                        size="xs"
                        trailing-icon="lucide:chevron-down"
                        :label="t('editQuery')"
                        class="px-0"
                    />
                    <template #content>
                        <UForm class="mt-2 space-y-4" @submit="search.fetchSearchResults">
                            <MonacoEditor
                                v-model="search.query.value"
                                :schema="choraleSearchQuerySchema"
                                :min-height="100"
                                :max-height="320"
                                :options="{
                                    fontSize: 14,
                                    tabSize: 12,
                                    scrollBeyondLastLine: false,
                                    automaticLayout: true,
                                    scrollbar: {
                                        alwaysConsumeMouseWheel: false,
                                    },
                                }"
                            />
                            <UButton v-if="search.pending.value || search.searchedQuery.value !== search.query.value" type="submit" :loading="search.pending.value">{{ t('submit') }}</UButton>
                        </UForm>
                    </template>
                </UCollapsible>
                <SearchResults :search="search" />
            </template>
        </UDrawer>
    </div>
</template>
