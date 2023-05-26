<script setup>
import levenshtein from 'js-levenshtein';
const { t } = useI18n();
const localePath = useLocalePath();
const route = useRoute();

// get chorale by nr url param
const { data } = await useAsyncData(`/bach-370-chorales/${route.params.nr}`, () => queryContent('/bach-370-chorales').where({nr: parseInt(route.params.nr, 10)}).findOne())
const chorale = createBachChorale(data.value);

// get all chorales to compare cantus firmi
const { data: choralesData } = await useAsyncData('/bach-370-chorales', () => queryContent('/bach-370-chorales').find())
const chorales = createBachChorales(choralesData.value);

// check for similar cantus firmi with levenshtein distance
const similarities = chorales.map((c) => ({
    id: c.id,
    distance: levenshtein(chorale.cantusFirmusMint, c.cantusFirmusMint),
})).filter(s => s.distance <= 20);

// get similar chorales
const { data: similarChoralesData } = await useAsyncData(`/bach-370-chorales/${route.params.nr}/similar`, () => queryContent('/bach-370-chorales').where({ id: { $ne: chorale.id, $in: similarities.map(s => s.id) } }).find())
const similarChorales = createBachChorales(similarChoralesData.value);

// get next and pevious chorales
const { data: surroundData } = await useAsyncData(`/bach-370-chorales/${route.params.nr}/surround`, () => queryContent('/bach-370-chorales').only(['_path', 'id', 'nr']).findSurround(data.value._path))
const [prevChorale, nextChorale] = surroundData.value;

useHead({
    title: `${chorale.nr}. ${chorale.title} | ${t('bach')}`,
});

const verovioOptions = {
    spacingSystem: 24,
    spacingStaff: 20,
    topMarginHarm: 2,
    bottomMarginHarm: 2,
    // harmDist: 3,
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
                            <NuxtLink :to="localePath({ name: 'bach-nr', params: { nr: prevChorale.nr }, hash: $route.hash })">
                                <Icon name="heroicons:arrow-left-circle" class="text-xl" />
                            </NuxtLink>
                        </div>
                        <div v-if="nextChorale">
                            <NuxtLink :to="localePath({ name: 'bach-nr', params: { nr: nextChorale.nr }, hash: $route.hash })">
                                <Icon name="heroicons:arrow-right-circle" class="text-xl" />
                            </NuxtLink>
                        </div>
                    </div>
                </div>
            </div>
        </div>

        <NuxtErrorBoundary>
            <Suspense>
                <HumdrumInteractiveScore
                    :url="chorale.localRawFile"
                    :score-formatter="scoreFormatter"
                    :verovio-options="verovioOptions"
                >
                    <BachChoraleFilters :score-formatter="scoreFormatter" />
                </HumdrumInteractiveScore>
                <template #error="{ error }">
                    <AlertMessage>
                        <p>{{ error }}</p>
                    </AlertMessage>
                </template>
            </Suspense>
        </NuxtErrorBoundary>

        <section>
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
        </section>

        <section v-if="similarChorales.length">
            <Subheading>
                {{ $t('choralesWithSameCantusFirmus') }}
            </Subheading>
            <ul>
                <li v-for="similarChorale in similarChorales">
                    <NuxtLink :to="localePath({ name: 'bach-nr', params: { nr: similarChorale.nr } })">
                        {{ `${similarChorale.nr}. ${similarChorale.title}` }}
                        <span class="text-xs">
                            ({{ $t('levenshteinDistance') }}: {{ similarities.find(c => c.id === similarChorale.id)?.distance || 0 }})
                        </span>
                    </NuxtLink>
                </li>
            </ul>
        </section>

    </Container>
</template>
