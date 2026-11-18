// How the parts of an analysis result are written: its numbers in the locale of the page, and a
// group that is named after a category of one of its reads (see /docs/grouping) in words.
export function useAnalysisFormat(result) {
    const { t, te, locale } = useI18n();
    const percent = new Intl.NumberFormat(locale.value, { style: 'percent', maximumFractionDigits: 1 });
    // As the table writes them: without a separator of the thousands.
    const count = new Intl.NumberFormat(locale.value, { useGrouping: false });
    const categoryNames = computed(() => (toValue(result)?.groupBy ?? [])
        .flatMap((level) => level.reads.flatMap((read) => Object.keys(read.categories ?? {}))));
    return {
        percent: (share) => percent.format(share),
        count: (value) => count.format(value),
        // Categories of one's own group query have no translation.
        label: (label) => (categoryNames.value.includes(label) && te(`analysisCategory.${label}`) ? t(`analysisCategory.${label}`) : label),
    };
}
