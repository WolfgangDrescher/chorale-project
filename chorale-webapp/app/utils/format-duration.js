// A duration with four significant digits, as a measurement is rounded in physics: in
// milliseconds up to 9999 ("87.35 ms", "1235 ms"), in seconds above that ("25.43 s"). Always
// with a decimal point, whatever the language of the page.
const formatFourDigits = new Intl.NumberFormat('en', {
    minimumSignificantDigits: 4,
    maximumSignificantDigits: 4,
    useGrouping: false,
});

export function formatDuration(ms) {
    // 9999.5 ms and up would round to five digits in milliseconds.
    if (ms < 9999.5) return `${formatFourDigits.format(ms)} ms`;
    return `${formatFourDigits.format(ms / 1000)} s`;
}
