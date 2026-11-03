import { onKeyStroke } from '@vueuse/core';

// The space bar plays and pauses, unless something on the page has the focus and takes the key
// itself: a text field, the editor of a query, a button, a slider.
export function useScorePlayerKeyboardShortcuts(toggle) {
    onKeyStroke(' ', (event) => {
        if (document.activeElement && document.activeElement !== document.body) return;
        event.preventDefault();
        toggle();
    });
}
