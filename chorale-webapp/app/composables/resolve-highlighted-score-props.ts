interface NoteGroup {
    items: string[];
    color?: string;
}

interface Section {
    startLine: number;
    endLine: number;
    label?: Label | string | null;
    voice?: number;
}

interface SectionGroup {
    items: Section[];
    color?: string;
}

interface Line {
    lineNumber: number,
    label?: Label | string | null;
}

interface LineGroup {
    items: Line[] | number[];
    color?: string;
}

interface ResolvedLineGroup {
    items: Line[];
    color?: string;
}

interface Label {
    value: string;
    position?: 'top' | 'bottom';
}

export type NotesProp = NoteGroup[] | string[];
export type LinesProp = LineGroup[] | number[];
export type SectionsProp = SectionGroup[] | Section[];

// All 17 standard Tailwind colors (500 shade, 40% opacity), keyed by name so a color can
// also be picked explicitly by name (e.g. mapping queryId "green" to this exact color, see
// search.vue) rather than only by position.
export const highlightColorsByName: Record<string, string> = {
    red: 'rgb(239 68 68 / 0.4)',
    green: 'rgb(34 197 94 / 0.4)',
    orange: 'rgb(249 115 22 / 0.4)',
    amber: 'rgb(245 158 11 / 0.4)',
    yellow: 'rgb(234 179 8 / 0.4)',
    lime: 'rgb(132 204 22 / 0.4)',
    emerald: 'rgb(16 185 129 / 0.4)',
    teal: 'rgb(20 184 166 / 0.4)',
    cyan: 'rgb(6 182 212 / 0.4)',
    sky: 'rgb(14 165 233 / 0.4)',
    blue: 'rgb(59 130 246 / 0.4)',
    indigo: 'rgb(99 102 241 / 0.4)',
    violet: 'rgb(139 92 246 / 0.4)',
    purple: 'rgb(168 85 247 / 0.4)',
    fuchsia: 'rgb(217 70 239 / 0.4)',
    pink: 'rgb(236 72 153 / 0.4)',
    rose: 'rgb(244 63 94 / 0.4)',
};

// Same 17 colors as a plain array, reordered so that each successive entry is as different
// in hue as possible from every one before it (greedy farthest-point selection over hue,
// starting from yellow) -- so even a small number of combined queries (see search.vue)
// gets well-separated colors, not adjacent hues. Used as the fallback when a group doesn't
// set its own color, by array position.
export const defaultHighlightColors = [
    'yellow',
    'teal',
    'blue',
    'red',
    'lime',
    'cyan',
    'purple',
    'orange',
    'green',
    'sky',
    'pink',
    'emerald',
    'fuchsia',
    'indigo',
    'amber',
    'violet',
    'rose',
].map((name) => highlightColorsByName[name]);

export function useResolveHighlightedScoreProps(props: {
    notes?: NotesProp;
    lines?: LinesProp;
    sections?: SectionsProp;
    filters?: Array<string>,
}) {
    const lineShiftAmount = computed(() => {
        let lineShift = 0;
        if (props.filters?.includes('meter -f')) lineShift += 1;
        if (props.filters?.includes('deg -k1 --box -t')) lineShift += 1;
        if (props.filters?.includes('deg -k1 --box')) lineShift += 1;
        return lineShift;
    });

    const applyLineShift = (value: number): number => {
        return value + lineShiftAmount.value;
    }

    const applyLineShiftToNoteId = (id: string): string => {
        const match = id.match(/^L(\d+)F(\d+)$/);
        if (!match) return id;

        const [, line, field] = match;
        if (!line || !field) return id;

        const shiftedLine = parseInt(line, 10) + lineShiftAmount.value;
        return `L${shiftedLine}F${field}`;
    };

    function resolveLabel(input?: Label | string | null): Label | null {
        if (input == null) return null;
        let value = '';
        if (typeof input === 'object') {
            value = input.value?.trim() ?? '';
        } else if (typeof input === 'string') {
            value = input.trim();
        } else if (typeof input === 'number') {
            value = String(input);
        }
        if (value === '') return null;
        const label: Label = { value };
        if (typeof input === 'object' && input.position) {
            label.position = input.position;
        }
        return label;
    }

    function resolveVoice(voice?: number): number | undefined {
        return Number.isInteger(voice) && voice! > 0 ? voice : undefined;
    }

    const isValidLineField = (id: string) => /^L\d+F\d+$/.test(id);

    const resolvedNotes = computed<NoteGroup[]>(() => {
        const notes = props.notes ?? [];

        // case: string[]
        if (notes.length > 0 && typeof notes[0] === 'string') {
            const validIds = (notes as string[]).filter(isValidLineField).map(applyLineShiftToNoteId);
            return [
                {
                    items: validIds,
                    color: defaultHighlightColors[0],
                },
            ];
        }

        // case: NoteGroup[]
        return (notes as NoteGroup[]).map((group, index) => ({
            ...group,
            items: group.items.filter(isValidLineField).map(applyLineShiftToNoteId),
            color: group.color || defaultHighlightColors[index % defaultHighlightColors.length],
        }));
    });

    const isValidSection = (section: Section): boolean => {
        return (
            typeof section.startLine === 'number' &&
            typeof section.endLine === 'number' &&
            section.startLine >= 0 &&
            section.endLine >= section.startLine
        );
    };

    const resolvedSections = computed<SectionGroup[]>(() => {
        const sections = props.sections ?? [];

        // case: Section[]
        if (sections.length > 0 && sections[0] && 'startLine' in sections[0]) {
            const valid = (sections as Section[]).filter(isValidSection).map((s) => ({
                    startLine: applyLineShift(s.startLine)!,
                    endLine: applyLineShift(s.endLine)!,
                    label: resolveLabel(s.label),
                    voice: resolveVoice(s.voice),
                }));
            return [
                {
                    items: valid,
                    color: defaultHighlightColors[0],
                },
            ];
        }

        // case 2: SectionGroup[]
        return (sections as SectionGroup[]).map((group, index) => ({
            ...group,
            items: Array.isArray(group.items)
                ? group.items.filter(isValidSection).map((s) => ({
                    startLine: applyLineShift(s.startLine)!,
                    endLine: applyLineShift(s.endLine)!,
                    label: resolveLabel(s.label) ?? null,
                    voice: resolveVoice(s.voice),
                }))
                : [],
            color: group.color || defaultHighlightColors[index % defaultHighlightColors.length],
        }));
    });

    const isValidLineNumber = (n: number): boolean => Number.isInteger(n) && n >= 0;

    const resolvedLines = computed<ResolvedLineGroup[]>(() => {
        const lines = props.lines ?? [];

        // case: number[]
        if (lines.length > 0 && typeof lines[0] === 'number') {
            const valid = (lines as number[]).filter(isValidLineNumber).map((n) => ({
                lineNumber: applyLineShift(n)!,
                label: null,
            }));
            return [
                {
                    items: valid,
                    color: defaultHighlightColors[0],
                },
            ];
        }

        // case: LineGroup[]
        return (lines as LineGroup[]).map((group, index) => {
            const groupItems = group.items ?? [];
            let resolvedItems: Line[] = [];

            if (typeof groupItems[0] === 'number') {
                // case: number[]
                resolvedItems = (groupItems as number[]).filter(isValidLineNumber).map((n) => ({
                    lineNumber: applyLineShift(n)!,
                    label: null,
                }));
            } else {
                // case: Line[]
                resolvedItems = (groupItems as Line[]).filter((l) => isValidLineNumber(l.lineNumber)).map((l) => ({
                    lineNumber: applyLineShift(l.lineNumber)!,
                    label: resolveLabel(l.label),
                }));
            }

            return {
                ...group,
                items: resolvedItems,
                color: group.color || defaultHighlightColors[index % defaultHighlightColors.length],
            };
});
    });

    return {
        defaultHighlightColors,
        resolvedNotes,
        resolvedSections,
        resolvedLines,
    };
}
