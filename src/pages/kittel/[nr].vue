<script setup>
const { t } = useI18n();
const localePath = useLocalePath();

const route = useRoute();
const { data } = await useAsyncData(`/kittel-24-chorales/${route.params.nr}`, () => queryContent('/kittel-24-chorales').where({ nr: parseInt(route.params.nr, 10) }).findOne())
const chorale = createKittelChorale(data.value);

const { data: surroundData } = await useAsyncData(`/kittel-24-chorales/${route.params.nr}/surround`, () => queryContent('/kittel-24-chorales').only(['_path', 'id', 'nr']).findSurround(data.value._path))
const [prevChorale, nextChorale] = surroundData.value;

useHead({
    title: `${chorale.nr}. ${chorale.title} | ${t('kittel')}`,
});

const verovioOptions = {
    spacingSystem: 24,
    spacingStaff: 20,
    topMarginHarm: 2,
    bottomMarginHarm: 2,
};

const kern = ref('');

onMounted(() => {
    fetch(chorale.localRawFile).then(response => {
        return response.text();
    }).then(value => {
        kern.value = value;
    });
});

const scoreFormatter = useHumdrumScoreFormatter();
</script>

<template>
    <Container>
        <div class="flex">
            <div class="flex-auto">
                <div class="mb-4">
                    <Heading class="mb-0">{{ `${chorale.nr}. ${chorale.title}` }}</Heading>
                    <div class="flex flex-gap-4">
                        <div v-if="prevChorale">
                            <NuxtLink :to="localePath({ name: 'kittel-nr', params: { nr: prevChorale.nr }, hash: $route.hash })">
                                <Icon name="heroicons:arrow-left-circle" class="text-xl" />
                            </NuxtLink>
                        </div>
                        <div v-if="nextChorale">
                            <NuxtLink :to="localePath({ name: 'kittel-nr', params: { nr: nextChorale.nr }, hash: $route.hash })">
                                <Icon name="heroicons:arrow-right-circle" class="text-xl" />
                            </NuxtLink>
                        </div>
                    </div>
                </div>
            </div>
        </div>

        <NuxtErrorBoundary>
            <Suspense>
                <HumdrumInteractiveScore :url="chorale.localRawFile" :score-formatter="scoreFormatter" :verovio-options="verovioOptions">
                    <BachChoraleFilters :score-formatter="scoreFormatter" />
                </HumdrumInteractiveScore>
                <template #error="{ error }">
                    <AlertMessage>
                        <p>{{ error }}</p>
                    </AlertMessage>
                </template>
            </Suspense>
        </NuxtErrorBoundary>

        <Subheading>
            {{ $t('kern') }}
        </Subheading>

        <div class="h-[300px] overflow-hidden">
            <MonacoEditor :model-value="kern" :options="{
                readOnly: true,
                fontSize: 14,
                // theme: 'vs-light',
                tabSize: 12,
                scrollBeyondLastLine: false,
                automaticLayout: true,
                scrollbar: {
                    alwaysConsumeMouseWheel: false,
                }
            }" />
        </div>

    </Container>
</template>
