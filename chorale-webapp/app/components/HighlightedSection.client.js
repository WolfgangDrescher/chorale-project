function getStaffIndexInMeasure(noteElem) {
    const staffElem = noteElem?.closest('.staff');
    const measureElem = staffElem?.closest('.measure');
    if (!staffElem || !measureElem) return null;
    const index = [...measureElem.querySelectorAll('.staff')].indexOf(staffElem);
    return index === -1 ? null : index;
}

// Finds any note/rest/whole-measure-rest belonging to `voice` inside `scopeElem` (a .measure
// or .system), purely to read off which staff that voice occupies there. Used as a fallback
// when the driving feature's own onset landed on a line where the target voice has no data
// record of its own -- e.g. a hint-<pair> spine changes because the OTHER voice in the pair
// moved while this one is still mid-note, so there's nothing at that exact line to anchor to
// directly, but the voice is still present (and occupies a fixed staff) throughout the piece.
function findVoiceStaffIndex(scopeElem, voice) {
    if (!scopeElem || voice == null) return null;
    const candidates = scopeElem.querySelectorAll('g[id^="note-L"], g[id^="rest-L"], g[id^="mrest-L"]');
    for (const elem of candidates) {
        const match = elem.id.match(/^(?:note|rest|mrest)-L\d+F(\d+)/);
        if (match && Number(match[1]) === voice) return getStaffIndexInMeasure(elem);
    }
    return null;
}

function getVoiceStaffRect(systemElem, voiceStaffIndex) {
    const staffsInFirstMeasure = systemElem?.querySelector('.measure')?.querySelectorAll('.staff');
    return getBBoxElem(staffsInFirstMeasure?.[voiceStaffIndex])?.getBoundingClientRect();
}

// Verovio keeps the accidental in its own <g class="accid"> next to the notehead, so the
// note's own bounding box starts at the notehead and cuts the accidental off. Return the
// accidental's box so the marker's left edge can include it.
function getAccidRect(elem) {
    const accidElem = elem?.querySelector?.('.accid:not(.bounding-box)');
    const rect = getBBoxElem(accidElem)?.getBoundingClientRect();
    return rect?.width > 0 ? rect : null;
}

function createMarker(startElem, endElem, systemElem, containerElem, color, voiceStaffIndex = null, outline = false) {
    const startAccidRect = getAccidRect(startElem);
    endElem = getBBoxElem(endElem) || endElem;

    const systemFirstMeasureStaffRect = selectBBoxElem(systemElem, '.measure .staff')?.getBoundingClientRect();
    const systemRect = getBBoxElem(systemElem)?.getBoundingClientRect();
    const containerRect = containerElem.getBoundingClientRect();
    const startRect = getBBoxElem(startElem)?.getBoundingClientRect();
    const endRect = getBBoxElem(endElem)?.getBoundingClientRect();

    const voiceStaffRect = voiceStaffIndex != null ? getVoiceStaffRect(systemElem, voiceStaffIndex) : null;
    const staffs = systemElem?.querySelectorAll('.measure .staff');
    const firstStaffRect = voiceStaffRect ?? getBBoxElem(staffs[0])?.getBoundingClientRect();
    const lastStaffRect = voiceStaffRect ?? getBBoxElem(staffs[staffs.length - 1])?.getBoundingClientRect();

    // An outline sits a little further out than a fill, so its 5px border starts at the fill's edge
    // of another marker underneath instead of leaving a white gap to it.
    const heightExtender = outline ? 20 : 15;
    const height = lastStaffRect.y + lastStaffRect.height - firstStaffRect.y  + heightExtender;

    const xPosStart = startRect
        ? Math.min(startRect.x, startAccidRect?.x ?? startRect.x)
        : (systemFirstMeasureStaffRect ? systemFirstMeasureStaffRect.x: systemRect.x);
    const xPosEnd = endRect ? endRect.right : getBBoxElem([...systemElem.querySelectorAll('.measure:not(.bounding-box)')].at(-1).querySelector('.staff:not(.bounding-box)'))?.getBoundingClientRect().right;

    const widthExtender = outline ? 20 : 15;
    const width = xPosEnd - xPosStart + widthExtender;
    const xOffset = 2;

    return h('div', {
        class: [
            'absolute',
            !outline && !startElem && 'bg-zig-zag-left',
            !outline && !endElem && 'bg-zig-zag-right',
            startElem && !endElem && 'rounded-tl rounded-bl',
            !startElem && endElem && 'rounded-tr rounded-br',
            startElem && endElem && 'rounded',
            // A system the section carries on from or into has no edge on that side.
            outline && 'z-30 border-[5px] border-solid',
            outline && !startElem && 'border-l-0',
            outline && !endElem && 'border-r-0',
        ],
        style: {
            ...(outline
                ? { borderColor: color }
                : { backgroundColor: color, '--zig-zag-color': color }),
            width: `${width}px`,
            height: `${height}px`,
            left: `${xPosStart - (widthExtender / 2) - containerRect.x + xOffset}px`,
            top: `${firstStaffRect.y - (heightExtender / 2) - containerRect.y}px`,
        },
    });
}

// A frame around exactly the given notes of one system, from the outermost edge of any of them to
// the outermost on each of the four sides, instead of around whole staves. A note is all of its
// parts: notehead, stem, flag, dots and accidental, which the bounding box of the note alone
// doesn't always cover.
function createFittedMarker(noteElems, containerElem, color) {
    const containerRect = containerElem.getBoundingClientRect();
    const rects = noteElems
        .flatMap((elem) => [
            getBBoxElem(elem)?.getBoundingClientRect(),
            elem.getBoundingClientRect(),
            ...[...elem.querySelectorAll('.notehead, .stem, .flag, .dots, .accid')].map((part) => part.getBoundingClientRect()),
        ])
        .filter((rect) => rect && (rect.width > 0 || rect.height > 0));
    if (!rects.length) return null;

    const left = Math.min(...rects.map((rect) => rect.left));
    const top = Math.min(...rects.map((rect) => rect.top));
    const right = Math.max(...rects.map((rect) => rect.right));
    const bottom = Math.max(...rects.map((rect) => rect.bottom));

    // The frame lies a little outside the notes, so its border doesn't touch them.
    const padding = 8;
    return h('div', {
        class: 'absolute z-30 rounded border-[5px] border-solid',
        style: {
            borderColor: color,
            width: `${right - left + padding * 2}px`,
            height: `${bottom - top + padding * 2}px`,
            left: `${left - padding - containerRect.x}px`,
            top: `${top - padding - containerRect.y}px`,
        },
    });
}

function selectBBoxElem(elem, selectors) {
    const selectedElem = elem?.querySelector(selectors);
    return getBBoxElem(selectedElem);
}

function getBBoxElem(elem) {
    return elem?.closest('svg')?.querySelector(`#bbox-${elem?.id} rect`) ?? elem;
}

export default {
    name: 'HighlightedSection',
    props: {
        startLine: Number,
        endLine: Number,
        voice: {
            type: Number,
            default: null,
        },
        color: String,
        // Draws the section as a frame around the notes instead of filling it.
        outline: Boolean,
        // Fits the frame to the notes of the section (those of `voice`, or of every voice) instead of
        // spanning the staves they are on: one frame per system, around exactly those notes.
        fitBoundingBox: Boolean,
        container: HTMLElement,
        label: {
            type: Object,
            default: null,
        },
    },
    setup(props) {

        const markers = [];

        let startElem = null;
        let endElem = null;
        const containerElem = props.container;

        if (props.fitBoundingBox && containerElem) {
            const noteElemsBySystem = new Map();
            const add = (noteElem) => {
                const system = noteElem.closest('g.system');
                noteElemsBySystem.set(system, [...(noteElemsBySystem.get(system) ?? []), noteElem]);
            };
            for (let line = props.startLine; line <= props.endLine; line++) {
                const suffix = props.voice != null ? `L${line}F${props.voice}` : `L${line}F`;
                containerElem.querySelectorAll(`g[id^="note-${suffix}"]`).forEach(add);
            }
            // A voice that attacks nothing there is framed by the note it holds.
            if (!noteElemsBySystem.size && props.voice != null) {
                const held = soundingNoteElem(containerElem, props.startLine, props.voice);
                if (held) add(held);
            }
            const fittedMarkers = [...noteElemsBySystem.values()]
                .map((noteElems) => createFittedMarker(noteElems, containerElem, props.color))
                .filter(Boolean);
            return () => h('div', {}, fittedMarkers);
        }
        const noteSelector = (line, voice) => {
            const suffix = voice != null ? `L${line}F${voice}` : `L${line}F`;
            return `g[id^="note-${suffix}"], g[id^="rest-${suffix}"], g[id^="mrest-${suffix}"]`;
        };

        for (let i = props.startLine; i <= props.endLine; i++) {
            startElem = props.container?.querySelector(noteSelector(i, props.voice));
            if (startElem) break;
        }

        for (let i = props.endLine; i >= props.startLine; i--) {
            endElem = props.container?.querySelector(noteSelector(i, props.voice));
            if (endElem) break;
        }

        // The target voice has no onset anywhere in [startLine, endLine] -- e.g. a driving
        // feature shared between two voices (hint-<pair>) changed because the OTHER voice in
        // the pair moved, while this one is still mid-note. Fall back to whichever voice DOES
        // have something there for horizontal/system positioning, but keep the highlight on
        // the correct voice's own staff (see findVoiceStaffIndex) rather than the fallback
        // voice's -- anchoring it to the wrong voice would misrepresent the match.
        let usedFallbackVoice = false;
        if (props.voice != null && !startElem && !endElem) {
            for (let i = props.startLine; i <= props.endLine; i++) {
                startElem = props.container?.querySelector(noteSelector(i, null));
                if (startElem) break;
            }
            for (let i = props.endLine; i >= props.startLine; i--) {
                endElem = props.container?.querySelector(noteSelector(i, null));
                if (endElem) break;
            }
            usedFallbackVoice = Boolean(startElem || endElem);
        }

        const voiceStaffIndex = props.voice == null
            ? null
            : usedFallbackVoice
                ? findVoiceStaffIndex((startElem ?? endElem)?.closest('.measure') ?? (startElem ?? endElem)?.closest('.system'), props.voice)
                : getStaffIndexInMeasure(startElem) ?? getStaffIndexInMeasure(endElem);

        if (startElem && endElem && containerElem) {

            const startSystem = startElem.closest('g.system');
            const endSystem = endElem.closest('g.system');

            if (startSystem === endSystem) {
                markers.push(createMarker(startElem, endElem, startSystem, containerElem, props.color, voiceStaffIndex, props.outline));
            } else {
                const systemParentChildren = startSystem.parentElement.children;
                const startIndex = [...systemParentChildren].indexOf(startSystem);
                const endIndex = [...systemParentChildren].indexOf(endSystem);

                for (let i = startIndex; i <= endIndex; i++) {
                    const systemElem = systemParentChildren[i];
                    markers.push(createMarker(
                        i === startIndex ? startElem : null,
                        i === endIndex ? endElem : null,
                        systemElem,
                        containerElem,
                        props.color,
                        voiceStaffIndex,
                        props.outline,
                    ));
                }
            }
        }

        let labelElem = null;
        if (props.label && props.label.value && markers.length > 0) {
            const firstMarker = markers[0];

            const position = props.label.position || 'top';

            const baseClasses = [
                'absolute',
                'text-sm',
                'bg-white/90',
                'rounded-md',
                'shadow',
                'p-2',
                'py-1',
                'z-1',
                'border',
                // 'pointer-events-none',
                'hover:z-2'
            ];

            const style = {
                left: firstMarker.props.style.left,
                borderColor: props.color,
            };

            if (position === 'bottom') {
                style.top = `calc(${firstMarker.props.style.top} + ${firstMarker.props.style.height} + 0.4em)`;
            } else {
                style.top = `calc(${firstMarker.props.style.top} - 2.3rem)`;
            }

            labelElem = h('div', { class: baseClasses.join(' '), style }, props.label.value);
        }

        return () => h('div', {}, labelElem ? [labelElem, ...markers] : markers);
    },
};
