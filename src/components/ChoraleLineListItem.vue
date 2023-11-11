<script setup>
const { localePath } = useI18n();
const props = defineProps({
    choraleLine: {
        type: Object,
        required: true,
    },
});

const data = ref(null);
onMounted(async () => {
    const response = await fetch(`/kern/bach-cadences/${props.choraleLine.filename}`);
    if (!response.ok) {
        throw new Error(`${response.status} ${response.statusText}`);
    }
    data.value = await response.text();
});

const title = `${props.choraleLine.choraleId}, T. ${props.choraleLine.endMeasure} ♩ ${props.choraleLine.endMeasureBeat}`;
</script>

<template>
    <Card :title="title">
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
                @score-is-ready="verovioCanvasIsReady"
            />
        </div>
    </Card>
</template>
