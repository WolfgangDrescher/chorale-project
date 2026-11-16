// What takes the space bar itself: a text field, the editor of a query, a button, a slider.
const TAKES_SPACE = 'input, textarea, select, button, a[href], [contenteditable], [role="slider"], [role="button"], .monaco-editor';

// The fallbacks of the mounted components, the last mounted wins: a drawer over the page.
const fallbacks = [];

function onSpace(event) {
    if (event.key !== ' ' || document.activeElement?.closest(TAKES_SPACE)) return;
    event.preventDefault();
    toggleCurrentScorePlayer(fallbacks.at(-1)?.());
}

// The space bar plays and pauses the player used last, or else the player getFallback returns.
export function useScorePlayerKeyboardShortcuts(getFallback) {
    if (import.meta.server) return;
    if (!fallbacks.length) window.addEventListener('keydown', onSpace);
    fallbacks.push(getFallback);
    onScopeDispose(() => {
        fallbacks.splice(fallbacks.indexOf(getFallback), 1);
        if (!fallbacks.length) window.removeEventListener('keydown', onSpace);
    });
}
