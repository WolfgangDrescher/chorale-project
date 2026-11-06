<script setup>
import { storeToRefs } from 'pinia';
import { useKeyModifier } from '@vueuse/core';

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
const { instrument, tempo, volume, reverb, voices } = storeToRefs(settings);

// With Shift or Alt held, a control of a voice sets all voices.
const { getKbdKey } = useKbd();
const isShiftPressed = useKeyModifier('Shift', { initial: false });
const isAltPressed = useKeyModifier('Alt', { initial: false });
const targetVoices = (voice) => (isShiftPressed.value || isAltPressed.value ? VOICES : [voice]);

function setVoiceSetting(voice, key, value) {
    for (const target of targetVoices(voice)) voices.value[target][key] = value;
}

// The stops of the organ, the lowest first.
const organStopItems = Object.keys(ORGAN_STOPS).map(Number).sort((a, b) => b - a);

// At least one stop stays drawn in each voice.
function setOrganStop(targets, stop, isDrawn) {
    for (const voice of targets) {
        const { organStops } = voices.value[voice];
        if (isDrawn && !organStops.includes(stop)) voices.value[voice].organStops = [...organStops, stop];
        else if (!isDrawn && organStops.includes(stop) && organStops.length > 1) voices.value[voice].organStops = organStops.filter((drawn) => drawn !== stop);
    }
}

function toggleOrganStop(voice, stop) {
    setOrganStop(targetVoices(voice), stop, !voices.value[voice].organStops.includes(stop));
}

// Drawn in all voices (true), in some ('mixed', as aria-pressed has it) or in none (false).
function organStopStateInAllVoices(stop) {
    const drawing = VOICES.filter((voice) => voices.value[voice].organStops.includes(stop)).length;
    return drawing === VOICES.length || (drawing > 0 && 'mixed');
}

// Draws a stop in all voices, or, drawn in all, takes it off in all.
function toggleOrganStopInAllVoices(stop) {
    setOrganStop(VOICES, stop, organStopStateInAllVoices(stop) !== true);
}

function organStopButtonProps(state) {
    return {
        'color': state ? 'primary' : 'neutral',
        'variant': state === true ? 'solid' : state ? 'soft' : 'outline',
        'aria-pressed': state,
    };
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
                                        v-bind="organStopButtonProps(organStopStateInAllVoices(stop))"
                                        class="flex-1 justify-center"
                                        @click="toggleOrganStopInAllVoices(stop)"
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
                                        @click="setVoiceSetting(voice, 'isMuted', !voices[voice].isMuted)"
                                    />
                                </div>
                                <div class="grid grid-cols-[auto_1fr_3rem] items-center gap-x-3 gap-y-2 text-xs text-muted" :class="voices[voice].isMuted && 'opacity-50'">
                                    <span>{{ $t('playerVolume') }}</span>
                                    <USlider :model-value="voices[voice].volume" :min="0" :max="100" :step="5" size="xs" @update:model-value="(value) => setVoiceSetting(voice, 'volume', value)" />
                                    <span class="tabular-nums text-right">{{ voices[voice].volume }} %</span>
                                    <span>{{ $t('playerPan') }}</span>
                                    <USlider :model-value="voices[voice].pan" :min="-100" :max="100" :step="10" size="xs" @update:model-value="(value) => setVoiceSetting(voice, 'pan', value)" />
                                    <span class="tabular-nums text-right">{{ panLabel(voices[voice].pan) }}</span>
                                    <span>{{ $t('playerReverb') }}</span>
                                    <USlider :model-value="voices[voice].reverb" :min="0" :max="100" :step="10" size="xs" @update:model-value="(value) => setVoiceSetting(voice, 'reverb', value)" />
                                    <span class="tabular-nums text-right">{{ voices[voice].reverb }} %</span>
                                    <template v-if="instrument === 'church_organ'">
                                        <span>{{ $t('playerOrganStops') }}</span>
                                        <UFieldGroup size="xs" class="col-span-2">
                                            <UButton
                                                v-for="stop in organStopItems"
                                                :key="stop"
                                                :label="`${stop}'`"
                                                v-bind="organStopButtonProps(voices[voice].organStops.includes(stop))"
                                                class="flex-1 justify-center"
                                                @click="toggleOrganStop(voice, stop)"
                                            />
                                        </UFieldGroup>
                                    </template>
                                </div>
                            </div>
                            <div class="flex items-center gap-3">
                                <UButton :label="$t('playerSpreadVoices')" icon="lucide:headphones" color="neutral" variant="outline" size="xs" @click="spreadVoices()" />
                                <span class="text-xs text-muted">{{ $t('playerSpreadVoicesDescription') }}</span>
                            </div>
                            <p class="text-xs text-muted">
                                {{ $t('playerAllVoicesModifier', { shift: getKbdKey('shift'), alt: getKbdKey('alt') }) }}
                            </p>
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
