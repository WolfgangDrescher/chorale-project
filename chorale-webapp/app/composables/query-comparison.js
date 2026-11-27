// Two queries searched side by side, and what the draft finds differently from the base in each
// chorale: the units both find (unchanged), those only the base finds (lost) and those only the
// draft finds (added). The unit compared is a match, or a note at a position the queries name
// ("as": "$note").
//
// With expected notes (see useExpectedDiminutions) only their chorales are compared, note by
// note: each note says whether it is expected and whether uncertain, and the expected notes
// neither query finds are `missing`.
export function useQueryComparison(baseQuery, draftQuery, expectedChorales) {
    const pending = ref(false);
    const preferredUnit = ref('note');

    function createSearch(query) {
        const results = ref(null);
        const error = ref(null);
        async function search() {
            error.value = null;
            try {
                const response = await $fetch('/api/chorale-search', {
                    method: 'POST',
                    body: toValue(query),
                    headers: { 'content-type': 'application/json' },
                });
                results.value = response.results;
            } catch (e) {
                results.value = null;
                error.value = e;
            }
        }
        return { results, error, search };
    }

    const base = createSearch(baseQuery);
    const draft = createSearch(draftQuery);

    async function compare() {
        pending.value = true;
        await Promise.all([base.search(), draft.search()]);
        pending.value = false;
    }

    const compared = computed(() => !!base.results.value && !!draft.results.value);

    const hasNamedPositions = (results) => Object.values(results ?? {}).flat().some(({ positionLines }) => positionLines);
    const notesAvailable = computed(() => hasNamedPositions(base.results.value) && hasNamedPositions(draft.results.value));
    const hasExpectation = computed(() => toValue(expectedChorales).size > 0);
    // The expected notes are annotated note by note.
    const unit = computed(() => {
        if (hasExpectation.value) return 'note';
        return notesAvailable.value ? preferredUnit.value : 'match';
    });

    const noteKey = ({ voice, line }) => `${voice}:${line}`;

    // Each unit a chorale's results hold, under a key that tells it from the others.
    function collectUnits(choraleResults) {
        const units = new Map();
        for (const { voice, startLine, endLine, positionLines } of choraleResults ?? []) {
            if (unit.value === 'match') {
                units.set(`${voice}:${startLine}-${endLine}`, { voice, startLine, endLine });
                continue;
            }
            for (const line of Object.values(positionLines ?? {})) units.set(noteKey({ voice, line }), { voice, line });
        }
        return units;
    }

    // The units the base and the draft find in each chorale compared, and those expected there.
    function foundUnits() {
        if (hasExpectation.value) {
            return [...toValue(expectedChorales)].map(([choraleId, notes]) => ({
                choraleId,
                baseUnits: collectUnits(base.results.value[choraleId]),
                draftUnits: collectUnits(draft.results.value[choraleId]),
                expectedUnits: new Map(notes.map((note) => [noteKey(note), note])),
            }));
        }
        const choraleIds = [...new Set([...Object.keys(base.results.value), ...Object.keys(draft.results.value)])];
        return choraleIds.map((choraleId) => ({
            choraleId,
            baseUnits: collectUnits(base.results.value[choraleId]),
            draftUnits: collectUnits(draft.results.value[choraleId]),
        }));
    }

    const chorales = computed(() => {
        if (!compared.value) return [];
        return foundUnits()
            .sort((x, y) => x.choraleId.localeCompare(y.choraleId, undefined, { numeric: true }))
            .map(({ choraleId, baseUnits, draftUnits, expectedUnits }) => {
                const select = (units, keep) => [...units]
                    .filter(([key]) => keep(key))
                    .map(([key, found]) => (expectedUnits ? { ...found, expected: expectedUnits.has(key), uncertain: !!expectedUnits.get(key)?.uncertain } : found));
                return {
                    choraleId,
                    unchanged: select(baseUnits, (key) => draftUnits.has(key)),
                    lost: select(baseUnits, (key) => !draftUnits.has(key)),
                    added: select(draftUnits, (key) => !baseUnits.has(key)),
                    missing: select(expectedUnits ?? [], (key) => !baseUnits.has(key) && !draftUnits.has(key)),
                };
            });
    });

    const total = (change) => chorales.value.reduce((sum, chorale) => sum + chorale[change].length, 0);
    const totals = computed(() => ({ unchanged: total('unchanged'), lost: total('lost'), added: total('added') }));

    // How a query does against the expected notes, by the changes that hold what it finds and
    // what it does not: true positives, false positives and false negatives. An uncertain note is none of them.
    function accuracy(found, notFound) {
        const count = (changes, expected) => chorales.value.reduce(
            (sum, chorale) => sum + changes.flatMap((change) => chorale[change]).filter((note) => !note.uncertain && note.expected === expected).length,
            0,
        );
        return { truePositives: count(found, true), falsePositives: count(found, false), falseNegatives: count(notFound, true) };
    }
    const accuracies = computed(() => (hasExpectation.value ? {
        base: accuracy(['unchanged', 'lost'], ['added', 'missing']),
        draft: accuracy(['unchanged', 'added'], ['lost', 'missing']),
    } : null));

    return { base, draft, pending, compare, compared, preferredUnit, notesAvailable, unit, hasExpectation, chorales, totals, accuracies };
}
