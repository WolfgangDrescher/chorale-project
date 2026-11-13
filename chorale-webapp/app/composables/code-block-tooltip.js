// The tooltips of the buttons on a code block shift to stay within the block. Its bottom edge is
// moved far down, so a tooltip of a short block does not flip above its button. Rendered within
// the block, a closing tooltip under the pointer keeps the block hovered and its buttons visible.
export function useCodeBlockTooltipProps(blockRef) {
    return computed(() => ({
        delayDuration: 0,
        disableHoverableContent: true,
        arrow: true,
        content: { side: 'bottom', sideOffset: 4, collisionBoundary: blockRef.value, collisionPadding: { left: 8, right: 8, bottom: -1000 } },
        portal: false,
    }));
}
