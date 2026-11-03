<script setup>
import { storeToRefs } from 'pinia';

// The settings all players share, behind a button: the instrument, the tempo and whether the
// voices are spread. The props style the button.
defineProps({
    variant: { type: String, default: 'ghost' },
    size: { type: String, default: 'md' },
});

const { t } = useI18n();

const { instrument, tempo, volume, reverb, spreadVoices } = storeToRefs(useScorePlayerSettings());

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
