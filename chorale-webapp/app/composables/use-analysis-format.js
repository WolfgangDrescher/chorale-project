// How the parts of an analysis result are written: its numbers in the locale of the page, and a
// group that is named after a category of one of its reads (see analyses/README.md) in words.
export function useAnalysisFormat(result) {
    const { t, locale } = useI18n();
    const percent = new Intl.NumberFormat(locale.value, { style: 'percent', maximumFractionDigits: 1 });
    // As the table writes them: without a separator of the thousands.
    const count = new Intl.NumberFormat(locale.value, { useGrouping: false });
    const categoryNames = computed(() => (toValue(result)?.groupBy ?? [])
        .flatMap((level) => level.reads.flatMap((read) => Object.keys(read.categories ?? {}))));
    return {
        percent: (share) => percent.format(share),
        count: (value) => count.format(value),
        label: (label) => (categoryNames.value.includes(label) ? t(`analysisCategory.${label}`) : label),
    };
}
