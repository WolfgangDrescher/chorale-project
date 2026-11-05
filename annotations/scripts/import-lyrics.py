#!/usr/bin/env python3
"""Imports the chorale lyrics of the music21 corpus (encoded by Margaret Greentree).

The music21 chorales are numbered by BWV and differ in a few notes from bach-370-chorales. Each
source chorale is aligned with the music21 chorales its !!!SCT record names, and the syllables go
onto the soprano attacks that match. Writes one syllable or null per soprano attack and verse.

Usage:
    python3 annotations/scripts/import-lyrics.py bach-370-chorales/kern > annotations/bach-lyrics.json

Reports the music21 file, match ratio and lost syllables of every chorale on stderr.
"""

import argparse
import difflib
import glob
import json
import os
import re
import sys

import music21

KERN_PITCH = re.compile(r'([a-gA-G])\1*([#-]*)')


def kern_midi(token):
    m = KERN_PITCH.search(token)
    letter, accidentals = m.group(0)[0], m.group(2)
    count = len(m.group(0)) - len(accidentals)
    octave = 4 + count - 1 if letter.islower() else 3 - (count - 1)
    pc = {'c': 0, 'd': 2, 'e': 4, 'f': 5, 'g': 7, 'a': 9, 'b': 11}[letter.lower()]
    return 12 * (octave + 1) + pc + accidentals.count('#') - accidentals.count('-')


def soprano_attacks(path):
    """MIDI pitches and section labels of the attacks of the last **kern spine, and the expansion
    list of the sections, e.g. ["A", "A", "B"]. Without labels, the last repeat sign splits A and B."""
    attacks, sections, expansion = [], [], []
    repeat_end = None
    spine = None
    section = None
    with open(path, encoding='utf-8', errors='replace') as f:
        for line in f:
            line = line.rstrip('\n')
            if line.startswith('**'):
                spine = max(i for i, t in enumerate(line.split('\t')) if t == '**kern')
                continue
            if spine is None or not line or line[0] == '!':
                continue
            token = line.split('\t')[spine]
            if line[0] == '=':
                if ':|' in token:
                    repeat_end = len(attacks)
                continue
            if label := re.fullmatch(r'\*>\[(.*)\]', token):
                expansion = label.group(1).split(',')
            elif label := re.fullmatch(r'\*>([^\[]+)', token):
                section = label.group(1)
            if line[0] == '*' or token == '.' or 'r' in token or '_' in token or ']' in token:
                continue
            attacks.append(kern_midi(token))
            sections.append(section)
    if not expansion and repeat_end:
        sections = ['A' if i < repeat_end else 'B' for i in range(len(attacks))]
        expansion = ['A', 'A', 'B']
    return attacks, sections, expansion


def move_up_single_sections(verses, sections, expansion):
    """In a section played once, the verses with lyrics move up into the empty lines above, so the
    lyrics after a repeat stand in the first line. Endings (A1, A2) keep their lines."""
    for section in set(sections):
        is_ending = re.fullmatch(r'(.+?)\d+', section or '') and re.sub(r'\d+$', '', section) in expansion
        if expansion.count(section) != 1 or is_ending:
            continue
        indices = [i for i, s in enumerate(sections) if s == section]
        sung = [v for v in verses if any(v[i] for i in indices)]
        for n, verse in enumerate(verses):
            for i in indices:
                verse[i] = sung[n][i] if n < len(sung) else None


def syllable(text, syllabic):
    text = text.strip()
    if syllabic in ('begin', 'middle'):
        text += '-'
    if syllabic in ('middle', 'end'):
        text = '-' + text
    return text


def music21_attacks(path):
    """[(MIDI pitch, {verse: syllable})] of the soprano."""
    parts = music21.converter.parse(path).parts
    # Only the soprano has lyrics, but instruments may stand above it.
    part = next((p for p in parts if any(n.lyrics for n in p.recurse().notes)), parts[0])
    attacks = []
    for note in part.recurse().notes:
        if note.isChord:
            note = note.notes[-1]
        if note.duration.isGrace or (note.tie is not None and note.tie.type in ('stop', 'continue')):
            continue
        lyrics = {l.number: syllable(l.text, l.syllabic) for l in note.lyrics if l.text and l.text.strip()}
        attacks.append((note.pitch.midi, lyrics))
    return attacks


def bwv_numbers(sct):
    """{"41", "171"} for "BWV 41/6 & 171/6"."""
    return set(re.findall(r'(?<![/\d(])(\d+)', sct))


def align(source, candidate):
    """(ratio, {source index: candidate index}) of the best transposition."""
    best = (0, {})
    for shift in range(-12, 13):
        matcher = difflib.SequenceMatcher(None, source, [m + shift for m, _ in candidate], autojunk=False)
        ratio = matcher.ratio()
        if ratio <= best[0]:
            continue
        pairs = {}
        for op, i1, i2, j1, j2 in matcher.get_opcodes():
            # Same length: the same notes with other pitches.
            if op == 'equal' or (op == 'replace' and i2 - i1 == j2 - j1):
                pairs.update({i1 + k: j1 + k for k in range(i2 - i1)})
        best = (ratio, pairs)
    return best


def remove_stanza_numbers(verses):
    """chorale-generate numbers the stanzas itself: "1. An" becomes "An"."""
    for verse in verses:
        first = next((i for i, s in enumerate(verse) if s), None)
        if first is not None:
            verse[first] = re.sub(r'^\d+\.\s+', '', verse[first])


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('source_dir', help='the bach-370-chorales kern directory')
    parser.add_argument('--min-ratio', type=float, default=0.9,
                        help='how well the soprano of the music21 chorale has to match (0-1, default 0.9)')
    args = parser.parse_args()

    corpus = os.path.join(os.path.dirname(music21.__file__), 'corpus', 'bach')
    candidates = sorted(glob.glob(os.path.join(corpus, 'bwv*.mxl')) + glob.glob(os.path.join(corpus, 'bwv*.xml')))
    parsed = {}

    result = {}
    for path in sorted(glob.glob(os.path.join(args.source_dir, '*.krn'))):
        chorale_id = os.path.basename(path)[:-4]
        with open(path, encoding='utf-8', errors='replace') as f:
            sct = next((l.split(':', 1)[1].strip() for l in f if l.startswith('!!!SCT')), '')
        source, sections, expansion = soprano_attacks(path)

        best = None
        for candidate in candidates:
            if re.match(r'bwv(\d+)', os.path.basename(candidate)).group(1) not in bwv_numbers(sct):
                continue
            if candidate not in parsed:
                parsed[candidate] = music21_attacks(candidate)
            attacks = parsed[candidate]
            if not any(lyrics for _, lyrics in attacks):
                continue
            ratio, pairs = align(source, attacks)
            if best is None or ratio > best[0]:
                best = (ratio, pairs, candidate)

        if best is None:
            print(f'{chorale_id}\t{sct}\t-\tno music21 chorale with text', file=sys.stderr)
            continue
        ratio, pairs, candidate = best
        name = os.path.basename(candidate)
        if ratio < args.min_ratio:
            print(f'{chorale_id}\t{sct}\t{name}\t{ratio:.3f}\tleft out, the soprano does not match', file=sys.stderr)
            continue

        attacks = parsed[candidate]
        numbers = sorted({n for _, lyrics in attacks for n in lyrics})
        verses = [[attacks[pairs[i]][1].get(n) if i in pairs else None for i in range(len(source))] for n in numbers]
        move_up_single_sections(verses, sections, expansion)
        paired = set(pairs.values())
        lost = sum(len(lyrics) for j, (_, lyrics) in enumerate(attacks) if j not in paired)
        print(f'{chorale_id}\t{sct}\t{name}\t{ratio:.3f}\t{lost} syllable(s) without a note', file=sys.stderr)
        remove_stanza_numbers(verses)
        result[chorale_id] = {'source': f'music21 corpus/bach/{name}', 'verses': verses}

    # One line per verse keeps the diffs short.
    out = ['{']
    for n, (chorale_id, entry) in enumerate(result.items()):
        out.append(f'    "{chorale_id}": {{')
        out.append(f'        "source": {json.dumps(entry["source"])},')
        out.append('        "verses": [')
        verses = [json.dumps(v, ensure_ascii=False) for v in entry['verses']]
        out.append(',\n'.join(f'            {v}' for v in verses))
        out.append('        ]')
        out.append('    }' + (',' if n < len(result) - 1 else ''))
    out.append('}')
    print('\n'.join(out))


if __name__ == '__main__':
    main()
