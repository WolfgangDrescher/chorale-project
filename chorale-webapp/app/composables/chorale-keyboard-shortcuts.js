import { onKeyStroke } from '@vueuse/core';

function ignoreIfInput() {
    const el = document.activeElement;
    // A slider takes the arrow keys itself.
    return el && (['input', 'textarea'].includes(el.tagName.toLowerCase()) || el.isContentEditable || !!el.closest('[role="slider"]'));
}

export function useChoraleKeyboardShortcuts({ prevChorale, nextChorale, scoreOptions = useScoreOptions() } = {}) {
    const route = useRoute();
    const localePath = useLocalePath();

    const prev = computed(() => unref(prevChorale));
    const next = computed(() => unref(nextChorale));

    onKeyStroke('ArrowLeft', () => {
        if (ignoreIfInput() || !prev.value) return;
        navigateTo(localePath({ name: 'chorale-id', params: { id: prev.value.choraleId }, hash: route.hash }));
    });

    onKeyStroke('ArrowRight', () => {
        if (ignoreIfInput() || !next.value) return;
        navigateTo(localePath({ name: 'chorale-id', params: { id: next.value.choraleId }, hash: route.hash }));
    });

    onKeyStroke('h', () => {
        if (ignoreIfInput()) return;
        scoreOptions.showHorizontalViewMode = !scoreOptions.showHorizontalViewMode;
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

    onKeyStroke('r', () => {
        if (ignoreIfInput()) return;
        scoreOptions.reset();
    });
}
