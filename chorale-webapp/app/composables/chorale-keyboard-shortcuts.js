import { onKeyStroke } from '@vueuse/core';

function ignoreIfInput() {
    const el = document.activeElement;
    // A slider takes the arrow keys itself.
    return el && (['input', 'textarea'].includes(el.tagName.toLowerCase()) || el.isContentEditable || !!el.closest('[role="slider"]'));
}

// keysTaken (a value, a ref or a getter) hands the arrow keys and H to something else on the page, like
// the annotation of modulations.
export function useChoraleKeyboardShortcuts({ prevChorale, nextChorale, keysTaken = false, scoreOptions = useScoreOptions() } = {}) {
    const route = useRoute();
    const localePath = useLocalePath();

    const prev = computed(() => unref(prevChorale));
    const next = computed(() => unref(nextChorale));

    onKeyStroke('ArrowLeft', () => {
        if (ignoreIfInput() || toValue(keysTaken) || !prev.value) return;
        navigateTo(localePath({ name: 'chorale-id', params: { id: prev.value.choraleId }, hash: route.hash }));
    });

    onKeyStroke('ArrowRight', () => {
        if (ignoreIfInput() || toValue(keysTaken) || !next.value) return;
        navigateTo(localePath({ name: 'chorale-id', params: { id: next.value.choraleId }, hash: route.hash }));
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
