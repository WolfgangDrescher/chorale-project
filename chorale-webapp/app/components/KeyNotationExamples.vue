<script setup>
// Keys named in a notation of KEY_NOTATIONS, Humdrum as code with its rules.
const props = defineProps({
    notation: { type: String, required: true },
});

const { t } = useI18n();

const EXAMPLE_KEYS = [['E-', 'major'], ['b', 'minor'], ['g', 'dorian']];

const examples = computed(() => EXAMPLE_KEYS.map(([key, mode]) => keyNameInNotation(key, mode, props.notation, t)));
</script>

<template>
    <i18n-t v-if="notation === 'humdrum'" keypath="keyNotationHumdrumRules" tag="span" scope="global">
        <template #flat><ProseCode class="text-[0.8em]">-</ProseCode></template>
        <template #sharp><ProseCode class="text-[0.8em]">#</ProseCode></template>
        <template #examples>
            <template v-for="(example, index) in examples" :key="example">{{ index ? ', ' : '' }}<ProseCode class="text-[0.8em]">{{ example }}</ProseCode></template>
        </template>
    </i18n-t>
    <span v-else>{{ examples.join(', ') }}</span>
</template>
