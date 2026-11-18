---
title: Lässt sich unter eine 3–2–1-Kadenz im Cantus firmus der Bass 4–5–1 setzen?
answer: "Die 3. Skalenstufe im Cantus firmus bildet dabei über der 4. Bassstufe eine Septime. Bach nutzt die Wendung selten und führt die Septime fast immer als Durchgang oder Syncopatio ein."
verdict: yes
date: 2026-11-16
tags: [Kadenz, Außenstimmen, Dissonanz]
stat:
    value: "20"
    label: Kadenzen
    chorales: "15"
    query: seventh
queries:
    - id: seventh
      title: 3–2–1 im Cantus firmus über 4–5–1 im Bass
      query: |
                {
                    "feature": "mint",
                    "voices": "bass",
                    "pattern": [
                        { "mint": "*", "duration": "4", "hint-14": "7" },
                        { "mint": "+M2", "duration": "4" },
                        { "mint": "-P5", "fermata": true }
                    ],
                    "simultaneousWith": [
                        {
                            "feature": "mint",
                            "voices": "soprano",
                            "pattern": [
                                { "mint": "*" },
                                { "mint": "-2" },
                                { "mint": "-2", "fermata": true }
                            ]
                        }
                    ],
                    "mintAllowIntervalComplementation": "5",
                    "hintReduceCompound": true,
                    "metweightSkipUnclassified": true,
                    "simultaneousAlignment": "end"
                }
    - id: passing-quarter
      title: Septime als Durchgang in Vierteln
      query: |
                {
                    "feature": "mint",
                    "voices": "bass",
                    "pattern": [
                        { "mint": "*", "duration": "4", "hint-14": "7" },
                        { "mint": "+M2", "duration": "4" },
                        { "mint": "-P5", "fermata": true }
                    ],
                    "simultaneousWith": [
                        {
                            "feature": "mint",
                            "voices": "soprano",
                            "pattern": [
                                { "mint": "*" },
                                { "mint": "-2", "duration": "4" },
                                { "mint": "-2" },
                                { "mint": "-2", "fermata": true }
                            ]
                        }
                    ],
                    "mintAllowIntervalComplementation": "5",
                    "hintReduceCompound": true,
                    "metweightSkipUnclassified": true,
                    "simultaneousAlignment": "end"
                }
    - id: passing-eighth
      title: Septime als Durchgang in Achteln
      query: |
                {
                    "feature": "mint",
                    "voices": "bass",
                    "pattern": [
                        { "mint": "*", "duration": "4", "hint-14": "7" },
                        { "mint": "+M2", "duration": "4" },
                        { "mint": "-P5", "fermata": true }
                    ],
                    "simultaneousWith": [
                        {
                            "feature": "mint",
                            "voices": "soprano",
                            "pattern": [
                                { "mint": "*" },
                                {
                                    "!mint": ["-2", "P1"],
                                    "duration": "4"
                                },
                                { "mint": "-2" },
                                { "mint": "-2", "fermata": true }
                            ]
                        },
                        {
                            "feature": "mint",
                            "voices": "soprano",
                            "pattern": [
                                { "mint": "-2" }
                            ],
                            "metweightSkipUnclassified": false,
                            "simultaneousAlignment": "start"
                        }
                    ],
                    "mintAllowIntervalComplementation": "5",
                    "hintReduceCompound": true,
                    "metweightSkipUnclassified": true,
                    "simultaneousAlignment": "end"
                }
    - id: syncopation
      title: Septime als Syncopatio
      query: |
                {
                    "feature": "mint",
                    "voices": "bass",
                    "pattern": [
                        { "mint": "*", "duration": "4", "hint-14": "7" },
                        { "mint": "+M2", "duration": "4" },
                        { "mint": "-P5", "fermata": true }
                    ],
                    "simultaneousWith": [
                        {
                            "feature": "mint",
                            "voices": "soprano",
                            "pattern": [
                                { "mint": "*", "duration": "2" },
                                { "mint": "-2" },
                                { "mint": "-2", "fermata": true }
                            ],
                            "durationAllowSplitNotes": true
                        }
                    ],
                    "mintAllowIntervalComplementation": "5",
                    "hintReduceCompound": true,
                    "metweightSkipUnclassified": true,
                    "simultaneousAlignment": "end"
                }
    - id: leap
      title: Septime angesprungen
      query: |
                {
                    "feature": "mint",
                    "voices": "bass",
                    "pattern": [
                        { "mint": "*", "duration": "4", "hint-14": "7" },
                        { "mint": "+M2", "duration": "4" },
                        { "mint": "-P5", "fermata": true }
                    ],
                    "simultaneousWith": [
                        {
                            "feature": "mint",
                            "voices": "soprano",
                            "pattern": [
                                { "mint": "*" },
                                { "!mint": "P1", "duration": "4" },
                                { "mint": "-2" },
                                { "mint": "-2", "fermata": true }
                            ]
                        },
                        {
                            "feature": "mint",
                            "voices": "soprano",
                            "pattern": [
                                { "!mint": "-2" }
                            ],
                            "metweightSkipUnclassified": false,
                            "simultaneousAlignment": "start"
                        }
                    ],
                    "mintAllowIntervalComplementation": "5",
                    "hintReduceCompound": true,
                    "metweightSkipUnclassified": true,
                    "simultaneousAlignment": "end"
                }
    - id: seventh-major
      title: Große Septime in Dur
      query: |
                {
                    "feature": "mint",
                    "voices": "bass",
                    "pattern": [
                        { "mint": "*", "duration": "4", "hint-14": "M7" },
                        { "mint": "+M2", "duration": "4" },
                        { "mint": "-P5", "fermata": true }
                    ],
                    "simultaneousWith": [
                        {
                            "feature": "mint",
                            "voices": "soprano",
                            "pattern": [
                                { "mint": "*" },
                                { "mint": "-2" },
                                { "mint": "-2", "fermata": true }
                            ]
                        }
                    ],
                    "mintAllowIntervalComplementation": "5",
                    "hintReduceCompound": true,
                    "metweightSkipUnclassified": true,
                    "simultaneousAlignment": "end"
                }
    - id: seventh-minor
      title: Kleine Septime in Moll
      query: |
                {
                    "feature": "mint",
                    "voices": "bass",
                    "pattern": [
                        { "mint": "*", "duration": "4", "hint-14": "m7" },
                        { "mint": "+M2", "duration": "4" },
                        { "mint": "-P5", "fermata": true }
                    ],
                    "simultaneousWith": [
                        {
                            "feature": "mint",
                            "voices": "soprano",
                            "pattern": [
                                { "mint": "*" },
                                { "mint": "-2" },
                                { "mint": "-2", "fermata": true }
                            ]
                        }
                    ],
                    "mintAllowIntervalComplementation": "5",
                    "hintReduceCompound": true,
                    "metweightSkipUnclassified": true,
                    "simultaneousAlignment": "end"
                }
    - id: diminished-seventh
      title: Verminderte Septime über ♯4
      query: |
                {
                    "feature": "mint",
                    "voices": "bass",
                    "pattern": [
                        { "mint": "*", "duration": "4", "hint-14": "7" },
                        { "mint": "+m2", "duration": "4" },
                        { "mint": "-P5", "fermata": true }
                    ],
                    "simultaneousWith": [
                        {
                            "feature": "mint",
                            "voices": "soprano",
                            "pattern": [
                                { "mint": "*" },
                                { "mint": "-2" },
                                { "mint": "-2", "fermata": true }
                            ]
                        }
                    ],
                    "mintAllowIntervalComplementation": "5",
                    "hintReduceCompound": true,
                    "metweightSkipUnclassified": true,
                    "simultaneousAlignment": "end"
                }
---

Steht in der Kadenz im Cantus firmus :deg{v="3 2 1" upper} und im
Bass :deg{v="4 5 1" bass}, entsteht über der :deg{v="4" bass} eine Septime
zwischen den Außenstimmen. Die eigentliche Frage ist, ob diese Septime
satztechnisch vertretbar ist. Bach zeigt: ja, wenn sie gut eingeführt wird.

Gezählt sind die Kadenzen, in denen der Bass in Vierteln in die Fermate geht.
Verzierungen auf Zwischenschlägen werden dabei ignoriert.

## Behandlung der Septime

| Behandlung | Anz. Kadenzen | Query |
|---|---|---|
| Durchgang in Vierteln | 10 | :query-ref{show-title id="passing-quarter"} |
| Durchgang in Achteln | 1 | :query-ref{show-title id="passing-eighth"} |
| Syncopatio | 8 | :query-ref{show-title id="syncopation"} |
| Sprung | 1 | :query-ref{show-title id="leap"} |

### Durchgang

Meist erreicht der Sopran die Septime im Durchgang von der :deg{v="4" upper}
her. Oft löst sich die :fb{v="7"} aber bereits in der nächsten Achtel in
eine :fb{v="6"} auf, wodurch eine Anticipatio der :deg{v="2" upper} über
der :deg{v="4" bass} entsteht. Nur in `chor083`, T. 14, wird die Septime nicht
über einen Durchgang in Vierteln erreicht, sondern über einen in Achteln:
Die :deg{v="4" upper} davor ist eine Achtel auf dem Zwischenschlag.

::score-examples
:::score-example{chorale-id="chor017" measures="3-4" caption="chor017, Erschienen ist der herrliche Tag (BWV 145/5), T. 3–4"}
:::
:::score-example{chorale-id="chor297" measures="1-2" caption="chor297, Jesu, der du meine Seele (BWV 78/7), T. 1–2"}
:::
:::score-example{chorale-id="chor083" measures="13-14" caption="chor083, Jesu Leiden, Pein und Tod (BWV 245/14), T. 13–14"}
:::
::

### Syncopatio

Ebenso oft ist die Septime vorbereitet. Die :deg{v="3" upper} klingt schon über
dem Basston davor als Konsonanz, gehalten oder neu angeschlagen, und erst der
Schritt des Basses zur :deg{v="4" bass} macht sie zur Septime. In `chor013` und
`chor341` ist im Cantus firmus vor dem neuen Anschlag eine untere Wechselnote
eingeschoben. Die drei gehaltenen Fälle stehen im 3/4-Takt, die Septime fällt
dort auf die zweite Zählzeit.

::score-examples
:::score-example{chorale-id="chor047" measures="9-10" caption="chor047, Vater unser im Himmelreich (BWV 245/5 & 416), T. 9–10"}
:::
:::score-example{chorale-id="chor203" measures="6-7" caption="chor203, O Mensch, schau Jesum Christum an (BWV 403), T. 6–7"}
:::
::

### Sprung

Frei angesprungen wird die Septime nur einmal: In `chor292` springt der Sopran
eine Terz abwärts in sie hinein.

::score-example{chorale-id="chor292" measures="1-2" caption="chor292, Nimm von uns, Herr, du treuer Gott (BWV 101/7), T. 1–2"}
::

## Große, kleine und verminderte Septime

| Bassstufe | Septime | Anz. Kadenzen | Query |
|---|---|---|---|
| :deg{v="4" bass} in Dur | groß | 13 | :query-ref{show-title id="seventh-major"} |
| :deg{v="4" bass} in Moll | klein | 7 | :query-ref{show-title id="seventh-minor"} |
| :deg{v="4+" bass} | vermindert | 5 | :query-ref{show-title id="diminished-seventh"} |

In Dur bildet die 3. Skalenstufe über der 4. Bassstufe eine große Septime, in
Moll eine kleine.

Geht der Bass :deg{v="4+ 5 1" bass}, ist die Septime über :deg{v="4+" bass}
immer vermindert, die Wendung steht also nur in Moll.

::score-example{chorale-id="chor047" measures="11-12" caption="chor047, Vater unser im Himmelreich (BWV 245/5 & 416), T. 11–12"}
::
