# Analyses

Each file here is an analysis a lesson shows on its page of data: a query of the chorale search, and how its matches
are grouped. `make analyses` runs every file through `chorale-group` and writes the tables to
`content/analysis-results/`, one file each. It runs with `--verify`, so a table only ever states what its queries find.

```sh
chorale-group corpus/bach-370-chorales --verify --query-file outer-voice-intervals.json
```

A lesson names its analyses in its front matter (`analyses`), by the name of the file without `.json`.

## The file

```json
{
    "query": { "feature": "kern", "voices": "bass", "pattern": [{ "!kern": "r" }] },
    "groupBy": [
        { "reads": [{ "feature": "hint-14" }] },
        { "reads": [{ "feature": "metweight" }] }
    ],
    "examples": { "maxCount": 0 },
    "variants": {
        "phraseEnd": { "pattern": [{ "phrase": "end" }] }
    }
}
```

| Key        | Required | What it is                                                               |
| ---------- | -------- | ------------------------------------------------------------------------ |
| `query`    | yes      | A query of the chorale search (see the docs of the webapp). Its matches are what is counted. It cannot be driven by a set of voice pairs (`hint-**`). |
| `groupBy`  | yes      | The levels of the grouping, see below.                                   |
| `examples` | no       | The passages of the score shown as examples of each group, see below.    |
| `variants` | no       | The same analysis read another way, or of fewer matches, see below.      |

Unknown keys are rejected everywhere, with a message that says where.

## `groupBy`

A list of levels. The first level groups all matches by what its reads find, the most frequent group first. Every
further level groups each group of the level before it again: the second level of the example splits the matches of
each interval by the metric weight of the bass note.

| Key             | Required | What it is                                                          |
| --------------- | -------- | ------------------------------------------------------------------- |
| `reads`         | yes      | What the level reads at every match, see below. The values of all its reads together make a group. |
| `maxGroupCount` | no       | Keep this many groups, the most frequent.                           |
| `minMatchCount` | no       | Keep only groups with at least this many matches (default 1).       |

The matches of the groups a level leaves out are counted as `omittedMatchCount`, shown as "Übrige".

### A read

| Key                                | Values                                         | What it is |
| ---------------------------------- | ---------------------------------------------- | ---------- |
| `feature`                          | `deg`, `mint`, `metweight`, `duration`, a voice pair such as `hint-14` | What is read. Required. |
| `positions`                        | positions of the pattern, from 0               | Where it is read, in the voice searched; all positions where it is left out. A note answering several positions is read once, at the first of them. |
| `notesOfVoice`                     | a voice, such as `"bass"`                      | Where it is read instead: every note this voice attacks across the match, in its own rhythm. Only with `mint`, and not with `positions`. Where the voice holds a note into the match, or into its last note, the group is the same; its members tell the notes it holds apart, shown as a tie, `‿P1`. |
| `ignoreQuality`                    | `true`, `false`                                | `"-M2"` and `"-m2"` count as `"-2"`; a unison keeps its quality. For `mint` and voice pairs. |
| `mintAllowIntervalComplementation` | `true`, `false`                                | An interval counts as its complement, as the search compares them with `mintAllowIntervalComplementation: "*"`: `"-P5"` is `"+P4"`, and a unison or an octave repeats the note before it and is left out. Only with `notesOfVoice`. |
| `categories`                       | an object of categories                        | The values are put into categories, see below. |

A value is read, then folded within an octave where the query asks for `hintReduceCompound`, then read without its
quality, then put into its category.

### Categories

```json
"categories": {
    "perfect": ["P1", "P5", "P8"],
    "imperfect": ["m3", "M3", "m6", "M6"],
    "dissonant": "remaining"
}
```

A category is a list of values. The string `"remaining"` makes a category of every value in no list, at most once. A
value in no list and with no remaining category stays itself, a group of its own. A held note has no value and is in
no category, and neither has a note the feature has no value at, as the interval of a pair while the other voice rests;
ask for a value in the query (`"hint-14": "*"`) to leave such matches out. A value in two lists is rejected.

## A group that stands for several values

A group of `categories`, of `mintAllowIntervalComplementation` or of `notesOfVoice` counts several values: `perfect`
counts `P5` and `P8`, `+P4` counts `+P4` and `-P5`, and `+M2 -P5` counts the bass that holds a note into the match,
`‿+M2 -P5`. They are its `members`, the most frequent first, each with its own query, and their queries find the
matches of the group together. A category keeps its name, any other group is named after its most frequent member,
without the notes it holds.

## `examples`

| Key        | Values              | What it is |
| ---------- | ------------------- | ---------- |
| `maxCount` | a number, default 5 | How many examples each group shows; 0 for none. |

The examples of a group are the first match of each set of ornamented voices, the unornamented first, then the first
match of further chorales; a chorale at most once. An ornament is a note on an unclassified metric position (`u` in
`**metweight`), which a search with `metweightSkipUnclassified` leaves out. Only the voice searched and the voices of
`notesOfVoice` are looked at.

A group with groups below it shows the first example of each of them instead, and a group with members the example of
each member that is ornamented in the fewest voices.

## `variants`

Each variant is made alongside the analysis and written under its name in `variants` of the result. The names are
free; a page that shows the result offers the variants it knows by their names (see `AnalysisView.vue`). Variants are
not combined: two of them read together are a variant of their own, with both keys.

| Key             | Values                     | What it is |
| --------------- | -------------------------- | ---------- |
| `ignoreQuality` | `true`, `false`            | Every read of every level reads without the quality. |
| `pattern`       | a list of objects          | Keys added to the pattern of the query, position by position: `[{}, { "phrase": "end" }]` adds `"phrase"` to the second position. At most as long as the pattern. |

## The result

The result is a tree of groups. Its root holds all matches, and every level of `groupBy` splits each group into the
groups below it. A group counts its matches and the chorales they are in, and has the query that finds exactly its
matches, or `null` where it cannot be asked for. Where the analysis asks for them, it also has its members, its examples
and the number of matches the groups left out had. The result repeats `groupBy` and has each variant under its name.
`Grouping.hpp` describes every field.
