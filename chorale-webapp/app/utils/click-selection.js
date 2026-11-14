// A click that adds to the chosen ones or takes away from them, with Shift, Alt, Ctrl or Cmd held.
export function isMultiSelectClick(event) {
    return !!(event?.altKey || event?.ctrlKey || event?.metaKey || event?.shiftKey);
}

// The ids chosen after a click on `id`: only that one, or after a multi-select click `id` toggled.
export function clickSelection(chosen, id, event) {
    if (!isMultiSelectClick(event)) return [id];
    return chosen.includes(id) ? chosen.filter((other) => other !== id) : [...chosen, id];
}
