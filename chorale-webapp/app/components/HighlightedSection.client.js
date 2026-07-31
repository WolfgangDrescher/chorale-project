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

function createMarker(startElem, endElem, systemElem, containerElem, color, voiceStaffIndex = null) {
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

    const heightExtender = 15;
    const height = lastStaffRect.y + lastStaffRect.height - firstStaffRect.y  + heightExtender;

    const xPosStart = startRect
        ? Math.min(startRect.x, startAccidRect?.x ?? startRect.x)
        : (systemFirstMeasureStaffRect ? systemFirstMeasureStaffRect.x: systemRect.x);
    const xPosEnd = endRect ? endRect.right : getBBoxElem([...systemElem.querySelectorAll('.measure:not(.bounding-box)')].at(-1).querySelector('.staff:not(.bounding-box)'))?.getBoundingClientRect().right;

    const widthExtender = 15;
    const width = xPosEnd - xPosStart + widthExtender;
    const xOffset = 2;

    return h('div', {
        class: [
            'absolute',
            !startElem && 'bg-zig-zag-left',
            !endElem && 'bg-zig-zag-right',
            startElem && !endElem && 'rounded-tl rounded-bl',
            !startElem && endElem && 'rounded-tr rounded-br',
            startElem && endElem && 'rounded',
        ],
        style: {
            backgroundColor: color,
            '--zig-zag-color': color,
            width: `${width}px`,
            height: `${height}px`,
            left: `${xPosStart - (widthExtender / 2) - containerRect.x + xOffset}px`,
            top: `${firstStaffRect.y - (heightExtender / 2) - containerRect.y}px`,
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
                markers.push(createMarker(startElem, endElem, startSystem, containerElem, props.color, voiceStaffIndex));
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
