const DEGREE = /^[#b♯♭]*\d+[+\-#b♯♭]*$/;
const VOICE_NUMBERS = { 1: 1, b: 1, bass: 1, 2: 2, t: 2, tenor: 2, 3: 3, a: 3, alto: 3, 4: 4, s: 4, soprano: 4 };
const VOICE_NAMES = { 1: 'voiceBass', 2: 'voiceTenor', 3: 'voiceAlto', 4: 'voiceSoprano' };

function singleVoice(selector) {
    return VOICE_NUMBERS[String(selector ?? '').trim().toLowerCase()] ?? null;
}

// Hint pairs read two voices, any other read the voice searched.
function voiceOfRead(read, searchedVoice) {
    if (read.voice || read.notesOfVoice) return singleVoice(read.voice ?? read.notesOfVoice);
    return read.feature.startsWith('hint-') ? null : searchedVoice;
}

// How the parts of an analysis result are written: its numbers in the locale of the page, and a
// group that is named after a category of one of its reads (see /docs/grouping) in words.
export function useAnalysisFormat(result) {
    const { t, te, locale } = useI18n();
    const percent = new Intl.NumberFormat(locale.value, { style: 'percent', maximumFractionDigits: 1 });
    // As the table writes them: without a separator of the thousands.
    const count = new Intl.NumberFormat(locale.value, { useGrouping: false });
    const categoryNames = computed(() => (toValue(result)?.groupBy ?? [])
        .flatMap((level) => level.reads.flatMap((read) => Object.keys(read.categories ?? {}))));
    const levelVoices = computed(() => (toValue(result)?.groupBy ?? []).map(({ reads }) => {
        const searchedVoice = singleVoice(toValue(result).query?.voices);
        const voices = new Set(reads.map((read) => voiceOfRead(read, searchedVoice)));
        return voices.size === 1 ? [...voices][0] : null;
    }));
    // Per level: how its degrees are drawn, or null if it reads anything else.
    const degreeVoices = computed(() => (toValue(result)?.groupBy ?? []).map(({ reads: [read, ...others] }, level) => {
        if (others.length || read.feature !== 'deg' || read.categories) return null;
        return { bass: levelVoices.value[level] === 1, upper: levelVoices.value[level] === 4 };
    }));
    return {
        percent: (share) => percent.format(share),
        count: (value) => count.format(value),
        // Categories of one's own group query have no translation.
        label: (label) => (categoryNames.value.includes(label) && te(`analysisCategory.${label}`) ? t(`analysisCategory.${label}`) : label),
        levelTitle: (level) => {
            const voice = levelVoices.value[level];
            return voice ? t('analysisGroupOfVoice', { voice: t(VOICE_NAMES[voice]) }) : t('analysisGroup');
        },
        degreeVoice: (level, label) => (label?.split(' ').every((value) => DEGREE.test(value)) ? degreeVoices.value[level] ?? null : null),
    };
}
