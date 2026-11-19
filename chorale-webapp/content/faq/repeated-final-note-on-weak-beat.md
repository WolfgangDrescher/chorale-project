---
title: Wechselt der Bass den Ton, wenn der Cantus firmus seinen Schlusston auf leichter Zählzeit wiederholt?
answer: "Der Bass bleibt auf seinem Ton: Er wiederholt ihn, springt in die Oktave oder hält ihn aus."
verdict: no
date: 2026-11-17
tags: [Kadenz, Metrik, Bass]
stat:
    value: "58"
    label: Kadenzen ohne Tonwechsel im Bass
    chorales: "22"
    query: repeated-final-note
    confirm:
        - query: repeated-final-note
          matches: "58"
        - query: bass-changes-note
          matches: "0"
        - query: fermatas
          matches: "2264"
        - query: fermatas-on-weak-beat
          matches: "119"
        - query: bass-repeats-note
          matches: "36"
        - query: bass-leaps-octave
          matches: "14"
        - query: bass-leaps-octave-up
          matches: "2"
        - query: bass-holds-note
          matches: "8"
queries:
    - id: fermatas
      title: Fermaten im Cantus firmus
      query: |
                {
                    "feature": "kern",
                    "voices": "soprano",
                    "pattern": [
                        { "fermata": true }
                    ]
                }
    - id: fermatas-on-weak-beat
      title: Fermaten im Cantus firmus auf leichter Zählzeit
      query: |
                {
                    "feature": "kern",
                    "voices": "soprano",
                    "pattern": [
                        { "fermata": true, "metweight": "w" }
                    ]
                }
    - id: repeated-final-note
      title: Wiederholter Schlusston des Cantus firmus auf leichter Zählzeit
      query: |
                {
                    "feature": "mint",
                    "voices": "soprano",
                    "mintStartAtPreviousToken": true,
                    "pattern": [
                        { "fermata": true, "metweight": "w", "mint": "P1" }
                    ]
                }
    - id: bass-changes-note
      title: "Gegenprobe: Der Bass wechselt dabei den Ton (keine Treffer)"
      query: |
                {
                    "feature": "mint",
                    "voices": "soprano",
                    "mintStartAtPreviousToken": true,
                    "mintAllowIntervalComplementation": "*",
                    "simultaneousAlignment": "end",
                    "pattern": [
                        { "fermata": true, "metweight": "w", "mint": "P1" }
                    ],
                    "simultaneousWith": [
                        {
                            "feature": "mint",
                            "voices": "bass",
                            "pattern": [
                                { "!mint": "1" }
                            ]
                        }
                    ]
                }
    - id: bass-repeats-note
      title: Der Bass wiederholt seinen Ton
      query: |
                {
                    "feature": "mint",
                    "voices": "soprano",
                    "mintStartAtPreviousToken": true,
                    "simultaneousAlignment": "end",
                    "pattern": [
                        { "fermata": true, "metweight": "w", "mint": "P1" }
                    ],
                    "simultaneousWith": [
                        {
                            "feature": "mint",
                            "voices": "bass",
                            "metweightSkipUnclassified": true,
                            "pattern": [
                                { "mint": "P1" }
                            ]
                        }
                    ]
                }
    - id: bass-leaps-octave
      title: Der Basston springt eine Oktave
      query: |
                {
                    "feature": "mint",
                    "voices": "soprano",
                    "mintStartAtPreviousToken": true,
                    "simultaneousAlignment": "end",
                    "pattern": [
                        { "fermata": true, "metweight": "w", "mint": "P1" }
                    ],
                    "simultaneousWith": [
                        {
                            "feature": "mint",
                            "voices": "bass",
                            "metweightSkipUnclassified": true,
                            "pattern": [
                                { "mint": "P8" }
                            ]
                        }
                    ]
                }
    - id: bass-leaps-octave-up
      title: Der Basston springt eine Oktave aufwärts
      query: |
                {
                    "feature": "mint",
                    "voices": "soprano",
                    "mintStartAtPreviousToken": true,
                    "simultaneousAlignment": "end",
                    "pattern": [
                        { "fermata": true, "metweight": "w", "mint": "P1" }
                    ],
                    "simultaneousWith": [
                        {
                            "feature": "mint",
                            "voices": "bass",
                            "metweightSkipUnclassified": true,
                            "pattern": [
                                { "mint": "+P8" }
                            ]
                        }
                    ]
                }
    - id: bass-holds-note
      title: Der Bass hält den Ton aus
      query: |
                {
                    "feature": "mint",
                    "voices": "soprano",
                    "mintStartAtPreviousToken": true,
                    "simultaneousAlignment": "start",
                    "pattern": [
                        { "fermata": true, "metweight": "w", "mint": "P1" }
                    ],
                    "simultaneousWith": [
                        {
                            "feature": "kern",
                            "voices": "bass",
                            "pattern": [
                                { "fermata": true }
                            ]
                        }
                    ]
                }
---

Die meisten Fermaten stehen auf einer schweren oder halbschweren Zählzeit. Im
Cantus firmus fallen 119 von 2264 auf eine leichte, und in 58 davon wiederholt
er nur den Ton der Zählzeit davor. Die Kadenz ist mit der betonten Silbe schon
erreicht, die unbetonte Schlusssilbe bekommt denselben Ton noch einmal.

::score-example{chorale-id="chor008" measures="1-2"}
::

Der Bass behandelt die Stelle genauso und wechselt in keiner der 58 Kadenzen den
Ton:

| Bass | Anz. Kadenzen | Query |
|---|---|---|
| wiederholt den Ton | 36 | :query-ref{show-title id="bass-repeats-note"} |
| springt eine Oktave | 14 | :query-ref{show-title id="bass-leaps-octave"} |
| hält den Ton aus | 8 | :query-ref{show-title id="bass-holds-note"} |

Verglichen wird der Basston auf der leichten Zählzeit mit dem auf der Zählzeit
davor, Achtel auf Zwischenschlägen bleiben außer Acht. Nur in zwei der 14 Fälle
springt der Bass eine Oktave
aufwärts (:query-ref{show-title id="bass-leaps-octave-up"}).

Einzig in :chorale{id="chor244"}, T. 10, läuft der Bass in Achteln weiter: Er
umspielt seinen Ton mit der Unteroktave und geht unter der Fermate im Durchgang
zur nächsten Phrase. Sonst gibt es Bewegung nur in den Mittelstimmen, etwa wenn
ein Vorhalt erst unter der Fermate aufgelöst wird.

Für das Komponieren eigener Choräle heißt das: Wiederholt der Cantus firmus
seinen Schlusston auf leichter Zählzeit, gehört der Schlussklang (die Ultima)
der Kadenz bereits auf die Zählzeit davor.
