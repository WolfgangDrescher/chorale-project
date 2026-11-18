<script setup>
import { useLocalStorage } from '@vueuse/core';

const { t } = useI18n();

const localePath = useLocalePath();

useHead({
    title: t('grouping'),
});

const DEMO_QUERY = `{
    "query": {
        "feature": "kern",
        "voices": "bass",
        "hintReduceCompound": true,
        "pattern": [
            { "!kern": "r", "hint-14": "*" }
        ]
    },
    "groupBy": [
        { "reads": [{ "feature": "hint-14" }] },
        { "reads": [{ "feature": "metweight" }] }
    ],
    "examples": { "maxCount": 0 },
    "variants": {
        "ignoreQuality": { "ignoreQuality": true }
    }
}`;

const query = useLocalStorage('chorale-group-query', DEMO_QUERY);

const result = ref(null);
const durationMs = ref(null);
const error = ref(null);
const pending = ref(false);
const progress = ref(null);

let abortController = null;

function cancel() {
    abortController?.abort();
}

async function group() {
    cancel();
    abortController = new AbortController();
    pending.value = true;
    progress.value = null;
    error.value = null;
    result.value = null;
    durationMs.value = null;
    try {
        const response = await fetchWithProgress('/api/chorale-group', {
            body: query.value,
            signal: abortController.signal,
            onProgress: (event) => {
                progress.value = { ...progress.value, ...event };
            },
        });
        result.value = response.result;
        durationMs.value = response.durationMs;
    } catch (e) {
        if (e.name !== 'AbortError') error.value = e;
    } finally {
        abortController = null;
        pending.value = false;
    }
}

onBeforeUnmount(cancel);

// ?query= replaces the stored group query and runs it, as on the search page.
const route = useRoute();
const router = useRouter();

onMounted(() => {
    if (!query.value.trim()) query.value = DEMO_QUERY;
    if (typeof route.query.query !== 'string') return;
    const parsed = parseQueryParameter(route.query.query);
    query.value = parsed === undefined ? route.query.query : formatQuery(parsed);
    router.replace({ query: { ...route.query, query: undefined } });
    group();
});

const parsedQuery = computed(() => {
    try {
        return JSON.parse(query.value);
    } catch {
        return undefined;
    }
});

function formatEditorQuery() {
    query.value = formatQuery(parsedQuery.value);
}

function applyDemoQuery() {
    query.value = DEMO_QUERY;
}

const hasGroups = computed(() => !!result.value?.groups?.length);
</script>

<template>
    <UContainer>
        <Heading>{{ $t('grouping') }}</Heading>
        <p class="-mt-2 mb-4 text-sm text-muted">{{ $t('groupingScope', { name: BACH_CHORALES.name }) }}</p>

        <UCard class="mb-4">
            <UForm class="space-y-4" @submit="group">
                <UFormField :label="$t('groupQuery')">
                    <template #hint>
                        <div class="flex gap-1">
                            <UTooltip :text="$t('formatQueryInvalid')" :disabled="parsedQuery !== undefined">
                                <span>
                                    <UButton
                                        color="neutral"
                                        variant="ghost"
                                        size="sm"
                                        icon="lucide:wand-sparkles"
                                        :label="$t('formatQuery')"
                                        :disabled="parsedQuery === undefined"
                                        @click="formatEditorQuery"
                                    />
                                </span>
                            </UTooltip>
                        </div>
                    </template>
                    <MonacoEditor
                        v-model="query"
                        :schema="choraleGroupQuerySchema"
                        schema-uri="inmemory://schema/chorale-group-query.json"
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
                </UFormField>
                <UButton type="submit" :loading="pending" :trailing="true">{{ $t('submit') }}</UButton>
            </UForm>
        </UCard>

        <UAlert v-if="error" color="error" variant="subtle" :title="error.data?.message ?? $t('groupingError')" class="mb-8">
            <template v-if="error.data?.errors?.length" #description>
                <ul>
                    <li v-for="(msg, i) in error.data.errors" :key="i">{{ msg }}</li>
                </ul>
            </template>
        </UAlert>
        <SearchProgress v-if="pending" :progress="progress" class="mt-8">
            <UButton color="neutral" variant="soft" size="xs" icon="lucide:x" @click="cancel">{{ $t('cancel') }}</UButton>
        </SearchProgress>
        <UEmpty
            v-else-if="!hasGroups"
            :title="result ? $t('noResults') : undefined"
            :description="$t('groupingEmptyDescription')"
            icon="lucide:chart-bar-decreasing"
            class="md:w-1/2 lg:w-1/3 mx-auto"
            :actions="[
                {
                    icon: 'lucide:file-text',
                    label: $t('readDocs'),
                    to: localePath('/docs/grouping'),
                },
                {
                    icon: 'lucide:settings',
                    label: $t('applyDemoQuery'),
                    onClick: applyDemoQuery,
                    color: 'neutral',
                    variant: 'subtle',
                },
            ]"
        />
        <template v-else>
            <p v-if="durationMs !== null" class="text-sm text-dimmed tabular-nums">
                {{ $t('groupingDuration', { duration: formatDuration(durationMs) }) }}
            </p>
            <Suspense>
                <AnalysisView :result="result" />
            </Suspense>
        </template>
    </UContainer>
</template>
