<script setup>
import { storeToRefs } from 'pinia';

// The settings all players share, behind a button: the instrument, for the organ its stops, the
// tempo and whether the voices are spread. The props style the button.
defineProps({
    variant: { type: String, default: 'ghost' },
    size: { type: String, default: 'md' },
});

const { t } = useI18n();

const { instrument, tempo, volume, reverb, spreadVoices, organStops } = storeToRefs(useScorePlayerSettings());

// The stops of the organ, the lowest first. At least one stays drawn.
const organStopItems = Object.keys(ORGAN_STOPS).map(Number).sort((a, b) => b - a);
function toggleOrganStop(stop) {
    if (!organStops.value.includes(stop)) organStops.value = [...organStops.value, stop];
    else if (organStops.value.length > 1) organStops.value = organStops.value.filter((drawn) => drawn !== stop);
}

const instrumentItems = computed(() => SCORE_PLAYER_INSTRUMENTS.map(({ id, label }) => ({ value: id, label: t(label) })));
</script>

<template>
    <UPopover arrow>
        <UButton icon="lucide:sliders-horizontal" :aria-label="$t('playerSettings')" color="neutral" :variant="variant" :size="size" />
        <template #content>
            <div class="flex flex-col gap-4 p-4 w-64">
                <UFormField :label="$t('playerInstrument')">
                    <USelect v-model="instrument" :items="instrumentItems" class="w-full" />
                </UFormField>
                <UFormField v-if="instrument === 'church_organ'" :label="$t('playerOrganStops')">
                    <UFieldGroup size="sm" class="w-full">
                        <UButton
                            v-for="stop in organStopItems"
                            :key="stop"
                            :label="`${stop}'`"
                            :color="organStops.includes(stop) ? 'primary' : 'neutral'"
                            :variant="organStops.includes(stop) ? 'solid' : 'outline'"
                            :aria-pressed="organStops.includes(stop)"
                            class="flex-1 justify-center"
                            @click="toggleOrganStop(stop)"
                        />
                    </UFieldGroup>
                </UFormField>
                <UFormField :label="$t('playerVolume')">
                    <template #hint>
                        <span class="tabular-nums">{{ volume }} %</span>
                    </template>
                    <USlider v-model="volume" :min="0" :max="100" :step="5" class="mt-2" />
                </UFormField>
                <UFormField :label="$t('playerReverb')">
                    <template #hint>
                        <span class="tabular-nums">{{ reverb }} %</span>
                    </template>
                    <USlider v-model="reverb" :min="0" :max="100" :step="5" class="mt-2" />
                </UFormField>
                <UFormField :label="$t('playerTempo')">
                    <template #hint>
                        <span class="tabular-nums">♩ = {{ tempo }}</span>
                    </template>
                    <USlider v-model="tempo" :min="40" :max="160" :step="4" class="mt-2" />
                </UFormField>
                <USwitch v-model="spreadVoices" :label="$t('playerSpreadVoices')" :description="$t('playerSpreadVoicesDescription')" />
            </div>
        </template>
    </UPopover>
</template>
