<script setup>
const { t } = useI18n();
const route = useRoute();
const router = useRouter();

useHead({
    title: t('choraleAnalysis'),
});

// The tab sits in the address, so a link (and the reload button) lands on the same one. Each
// tab's panel is mounted once it has been opened and kept, so switching back doesn't lose an
// analysis that has already run.
const TABS = [
    { value: 'segments', label: t('segmentAnalysis'), icon: 'lucide:scissors', slot: 'segments' },
    { value: 'check', label: t('checkAnalysis'), icon: 'lucide:spell-check', slot: 'check' },
];

const tab = computed({
    get: () => (TABS.some((entry) => entry.value === route.query.tab) ? route.query.tab : TABS[0].value),
    set: (value) => router.replace({ query: { ...route.query, tab: value === TABS[0].value ? undefined : value } }),
});

// The score is the page's, not a tab's: the same file is analyzed in either.
const file = ref(null);
</script>

<template>
    <UContainer>
        <Heading>{{ $t('choraleAnalysis') }}</Heading>

        <UTabs v-model="tab" :items="TABS" :unmount-on-hide="false" class="w-full">
            <template #segments>
                <SegmentAnalysis v-model:file="file" :active="tab === 'segments'" class="mt-4" />
            </template>
            <template #check>
                <CheckAnalysis v-model:file="file" :active="tab === 'check'" class="mt-4" />
            </template>
        </UTabs>
    </UContainer>
</template>
