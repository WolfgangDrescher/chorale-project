<script setup>
import { useClipboard } from '@vueuse/core';

const localePath = useLocalePath();
const { params: { id } } = useRoute();

const { data: chorale } = await useAsyncData(`chorales/${id}`, () => queryCollection('chorales').where('stem', '=', `chorales/${id}`).first());

if (!chorale.value) {
    throw createError({
        statusCode: 404,
        statusMessage: 'Page Not Found',
    });
}

useHead({
    title: computed(() => chorale.value.title ? `${chorale.value.title} (${id})` : id),
});

const { data: surroundData } = await useAsyncData(`chorales/${id}/surround`, () => {
    return queryCollectionItemSurroundings('chorales', chorale.value.path, { fields: ['choraleId'] }).order('stem', 'ASC');
});
const prevChorale = computed(() => surroundData.value?.[0] ?? null);
const nextChorale = computed(() => surroundData.value?.[1] ?? null);

const { githubScoreUrlGenerator, vhvScoreUrlGenerator } = useScoreUrlGenerator();

const scoreOptions = useScoreOptions();

const { copy, copied } = useClipboard();

useChoraleKeyboardShortcuts({ prevChorale, nextChorale });

// The key as the score writes it, e.g. "G major" or "f# minor".
const keyName = computed(() => [chorale.value.key, chorale.value.mode].filter(Boolean).join(' '));
</script>

<template>
    <UContainer>
        <div class="flex flex-col gap-8">
            <div>
                <div class="mb-3">
                    <UBadge color="neutral" size="sm" variant="outline" class="font-mono cursor-pointer select-none w-[11ch] inline-flex items-center justify-center text-center" :label="copied ? $t('copied') : id" @click="copy(id)" />
                </div>
                <Heading>
                    {{ chorale.title ?? id }}
                    <div class="text-base font-normal">
                        {{ [chorale.composer, chorale.bwv, keyName, chorale.meter].filter(Boolean).join(' · ') }}
                    </div>
                </Heading>
                <div class="flex gap-2">
                    <UButton :disabled="!prevChorale" :to="localePath({ name: 'chorale-id', params: { id: prevChorale?.choraleId }, hash: $route.hash })" size="xs">
                        <template #leading>
                            <UKbd color="neutral">
                                <UIcon name="lucide:arrow-left" />
                            </UKbd>
                        </template>
                        {{ $t('previous') }}
                    </UButton>
                    <UButton :disabled="!nextChorale" :to="localePath({ name: 'chorale-id', params: { id: nextChorale?.choraleId }, hash: $route.hash })" size="xs">
                        {{ $t('next') }}
                        <template #trailing>
                            <UKbd color="neutral">
                                <UIcon name="lucide:arrow-right" />
                            </UKbd>
                        </template>
                    </UButton>
                </div>
            </div>

            <div class="flex flex-col md:flex-row items-center gap-4">
                <div>
                    <ScoreOptionsPalette />
                </div>
                <div class="shrink-0 flex gap-2 ml-auto md:order-2">
                    <UButton :to="githubScoreUrlGenerator(chorale.choraleId)" target="_blank">
                        {{ $t('github') }}
                    </UButton>
                    <UButton :to="vhvScoreUrlGenerator(chorale.choraleId)" target="_blank">
                        {{ $t('vhv') }}
                    </UButton>
                </div>
            </div>

            <HighlightedScore
                :piece-id="chorale.choraleId"
                :horizontal="scoreOptions.showHorizontalViewMode"
                :verovio-options="{
                    ...scoreOptions.verovioOptions,
                    header: 'none',
                    spacingSystem: 15,
                    pageMarginLeft: 50,
                    pageMarginRight: 0,
                    pageMarginTop: 50,
                    pageMarginBottom: 50,
                }"
                :filters="scoreOptions.humdrumFilters"
            />
        </div>
    </UContainer>
</template>
