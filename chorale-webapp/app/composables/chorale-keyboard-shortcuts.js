import { onKeyStroke } from '@vueuse/core';

function ignoreIfInput() {
    const el = document.activeElement;
    // A slider takes the arrow keys itself.
    return el && (['input', 'textarea'].includes(el.tagName.toLowerCase()) || el.isContentEditable || !!el.closest('[role="slider"]'));
}

// A key held with Cmd, Ctrl or Alt is a shortcut of the browser, like Cmd+R.
function onShortcut(key, handler) {
    onKeyStroke(key, (event) => {
        if (event.metaKey || event.ctrlKey || event.altKey) return;
        handler(event);
    });
}

// keysTaken (a value, a ref or a getter) hands the arrow keys and H to something else on the page, like
// the annotation of modulations.
// prevRoute and nextRoute are the pages of the neighbouring chorales, or null.
export function useChoraleKeyboardShortcuts({ prevRoute, nextRoute, keysTaken = false, scoreOptions = useScoreOptions() } = {}) {
    const prev = computed(() => toValue(prevRoute));
    const next = computed(() => toValue(nextRoute));

    onShortcut('ArrowLeft', () => {
        if (ignoreIfInput() || toValue(keysTaken) || !prev.value) return;
        navigateTo(prev.value);
    });

    onShortcut('ArrowRight', () => {
        if (ignoreIfInput() || toValue(keysTaken) || !next.value) return;
        navigateTo(next.value);
    });

    onShortcut('h', () => {
        if (ignoreIfInput() || toValue(keysTaken)) return;
        scoreOptions.toggleHorizontalViewMode();
    });

    onShortcut('l', () => {
        if (ignoreIfInput()) return;
        scoreOptions.showLyrics = !scoreOptions.showLyrics;
    });

    onShortcut('+', () => {
        if (ignoreIfInput()) return;
        scoreOptions.zoomIn();
    });

    onShortcut('-', () => {
        if (ignoreIfInput()) return;
        scoreOptions.zoomOut();
    });

    onShortcut('0', () => {
        if (ignoreIfInput()) return;
        scoreOptions.resetZoom();
    });

    onShortcut('m', () => {
        if (ignoreIfInput()) return;
        scoreOptions.showModulations = !scoreOptions.showModulations;
    });

    onShortcut('r', () => {
        if (ignoreIfInput()) return;
        scoreOptions.reset();
    });
}
