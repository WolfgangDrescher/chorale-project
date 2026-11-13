import { onKeyStroke } from '@vueuse/core';

function ignoreIfInput() {
    const el = document.activeElement;
    // A slider takes the arrow keys itself.
    return el && (['input', 'textarea'].includes(el.tagName.toLowerCase()) || el.isContentEditable || !!el.closest('[role="slider"]'));
}

// keysTaken (a value, a ref or a getter) hands the arrow keys and H to something else on the page, like
// the annotation of modulations.
// prevRoute and nextRoute are the pages of the neighbouring chorales, or null.
export function useChoraleKeyboardShortcuts({ prevRoute, nextRoute, keysTaken = false, scoreOptions = useScoreOptions() } = {}) {
    const prev = computed(() => toValue(prevRoute));
    const next = computed(() => toValue(nextRoute));

    onKeyStroke('ArrowLeft', () => {
        if (ignoreIfInput() || toValue(keysTaken) || !prev.value) return;
        navigateTo(prev.value);
    });

    onKeyStroke('ArrowRight', () => {
        if (ignoreIfInput() || toValue(keysTaken) || !next.value) return;
        navigateTo(next.value);
    });

    onKeyStroke('h', () => {
        if (ignoreIfInput() || toValue(keysTaken)) return;
        scoreOptions.showHorizontalViewMode = !scoreOptions.showHorizontalViewMode;
    });

    onKeyStroke('l', () => {
        if (ignoreIfInput()) return;
        scoreOptions.showLyrics = !scoreOptions.showLyrics;
    });

    onKeyStroke('+', () => {
        if (ignoreIfInput()) return;
        scoreOptions.zoomIn();
    });

    onKeyStroke('-', () => {
        if (ignoreIfInput()) return;
        scoreOptions.zoomOut();
    });

    onKeyStroke('0', () => {
        if (ignoreIfInput()) return;
        scoreOptions.resetZoom();
    });

    onKeyStroke('m', () => {
        if (ignoreIfInput()) return;
        scoreOptions.showModulations = !scoreOptions.showModulations;
    });

    onKeyStroke('r', () => {
        if (ignoreIfInput()) return;
        scoreOptions.reset();
    });
}
