<script setup>
import { storeToRefs } from 'pinia';

// The settings all players share, behind a button: the sound, volume and tempo of all voices, and
// the mix of each voice. Given a player, its score can be downloaded as MIDI as it sounds. The other
// props style the button.
defineProps({
    player: { type: Object, default: null },
    midiFileName: { type: String, default: 'score.mid' },
    variant: { type: String, default: 'ghost' },
    size: { type: String, default: 'md' },
});

const { t } = useI18n();

const settings = useScorePlayerSettings();
const { instrument, tempo, volume, reverb, organStops, voices } = storeToRefs(settings);

// The stops of the organ, the lowest first. At least one stays drawn.
const organStopItems = Object.keys(ORGAN_STOPS).map(Number).sort((a, b) => b - a);
function toggleOrganStop(stop) {
    if (!organStops.value.includes(stop)) organStops.value = [...organStops.value, stop];
    else if (organStops.value.length > 1) organStops.value = organStops.value.filter((drawn) => drawn !== stop);
}

const instrumentItems = computed(() => SCORE_PLAYER_INSTRUMENTS.map(({ id, label }) => ({ value: id, label: t(label) })));

const tabItems = computed(() => [
    { label: t('playerAllVoices'), slot: 'all' },
    { label: t('playerEachVoice'), slot: 'each' },
]);

const VOICE_LABELS = { soprano: 'voiceSoprano', alto: 'voiceAlto', tenor: 'voiceTenor', bass: 'voiceBass' };

function panLabel(pan) {
    if (!pan) return t('playerPanCenter');
    return t(pan < 0 ? 'playerPanLeft' : 'playerPanRight', { value: Math.abs(pan) });
}

function spreadVoices() {
    for (const voice of VOICES) voices.value[voice].pan = STEREO_SPREAD_PANS[voice];
}
</script>

<template>
    <UPopover arrow>
        <UButton icon="lucide:sliders-horizontal" :aria-label="$t('playerSettings')" color="neutral" :variant="variant" :size="size" />
        <template #content>
            <div class="flex flex-col gap-4 p-4 w-72">
                <UTabs :items="tabItems" size="xs" class="w-full" :ui="{ content: 'pt-2' }">
                    <template #all>
                        <div class="flex flex-col gap-4">
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
                        </div>
                    </template>
                    <template #each>
                        <div class="flex flex-col gap-3">
                            <div v-for="(label, voice) in VOICE_LABELS" :key="voice">
                                <div class="flex items-center justify-between">
                                    <span class="text-sm font-medium" :class="voices[voice].isMuted && 'text-muted'">{{ $t(label) }}</span>
                                    <UButton
                                        :icon="voices[voice].isMuted ? 'lucide:volume-x' : 'lucide:volume-2'"
                                        :aria-label="$t('playerMuteVoice')"
                                        :aria-pressed="voices[voice].isMuted"
                                        :color="voices[voice].isMuted ? 'primary' : 'neutral'"
                                        variant="ghost"
                                        size="xs"
                                        @click="voices[voice].isMuted = !voices[voice].isMuted"
                                    />
                                </div>
                                <div class="grid grid-cols-[auto_1fr_3rem] items-center gap-x-3 gap-y-2 text-xs text-muted" :class="voices[voice].isMuted && 'opacity-50'">
                                    <span>{{ $t('playerVolume') }}</span>
                                    <USlider v-model="voices[voice].volume" :min="0" :max="100" :step="5" size="xs" />
                                    <span class="tabular-nums text-right">{{ voices[voice].volume }} %</span>
                                    <span>{{ $t('playerPan') }}</span>
                                    <USlider v-model="voices[voice].pan" :min="-100" :max="100" :step="10" size="xs" />
                                    <span class="tabular-nums text-right">{{ panLabel(voices[voice].pan) }}</span>
                                    <span>{{ $t('playerReverb') }}</span>
                                    <USlider v-model="voices[voice].reverb" :min="0" :max="100" :step="10" size="xs" />
                                    <span class="tabular-nums text-right">{{ voices[voice].reverb }} %</span>
                                </div>
                            </div>
                            <div class="flex items-center gap-3">
                                <UButton :label="$t('playerSpreadVoices')" icon="lucide:headphones" color="neutral" variant="outline" size="xs" @click="spreadVoices()" />
                                <span class="text-xs text-muted">{{ $t('playerSpreadVoicesDescription') }}</span>
                            </div>
                        </div>
                    </template>
                </UTabs>
                <div class="flex items-center justify-between gap-2 pt-3 border-t border-default">
                    <UButton :label="$t('playerResetSettings')" icon="lucide:list-restart" color="neutral" variant="ghost" size="xs" @click="settings.reset()" />
                    <UButton
                        v-if="player"
                        :label="$t('downloadFormat', { format: 'MIDI' })"
                        icon="lucide:download"
                        color="neutral"
                        variant="outline"
                        size="xs"
                        :disabled="!player.hasScore"
                        @click="player.downloadMidi(midiFileName)"
                    />
                </div>
            </div>
        </template>
    </UPopover>
</template>
