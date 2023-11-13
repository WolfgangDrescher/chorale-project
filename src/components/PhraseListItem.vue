<script setup>
const props = defineProps({
    phrase: {
        type: Object,
        required: true,
    },
});

const localePath = useLocalePath();

const data = ref(null);
onMounted(async () => {
    const response = await fetch(`/kern/bach-phrases/${props.phrase.filename}`);
    if (!response.ok) {
        throw new Error(`${response.status} ${response.statusText}`);
    }
    data.value = await response.text();
});

const title = `${props.phrase.choraleId}, T. ${props.phrase.endMeasure} ♩ ${props.phrase.endMeasureBeat}`;
</script>

<template>
    <Card :title="title">
        <template v-slot:title>
            <div class="text-xl font-medium leading-5 text-gray-800">
                <NuxtLink :href="localePath({name: 'bach-chorale-nr', params: { nr: parseInt(phrase.choraleId.replaceAll(/\D/g, ''), 10) }})">
                    {{ title }}
                </NuxtLink>
            </div>
        </template>
        <div class="flex flex-col gap-4 mt-4">
            <VerovioCanvas
                v-if="data"
                :data="data"
                view-mode="horizontal"
                :scale="35"
                lazy
                unload
                :lazy-delay="100"
                :options="{ mnumInterval: 1 }"
            />
        </div>
    </Card>
</template>
