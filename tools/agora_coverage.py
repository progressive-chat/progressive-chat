#!/usr/bin/env python3
# SPDX-License-Identifier: AGPL-3.0-only
"""How much of Agora's port stands in this tree, counted.

The names come from `tools/agora-map.json` -- an index of every entity named
in the headers of `ports/agora-core`, prepared for the roadmap. A name counts
as found when its own spelling stands in `src/` or `test/` as a word, or when
`docs/agora-bridge.md` says what this tree calls the same thing and that name
stands there instead. The counting is by spelling, and nothing is guessed.

    tools/agora_coverage.py            say how much is found
    tools/agora_coverage.py --missing  say what is not
    tools/agora_coverage.py --min 80   fail under 80 (for a CI floor)
"""
import argparse
import json
import pathlib
import re
import sys

root = pathlib.Path(__file__).resolve().parent.parent


def source_files():
    out = []
    for pattern in ('src/**/*.cc', 'src/**/*.h', 'src/**/*.cpp',
                    'test/**/*.cc', 'test/*.h'):
        out.extend(root.glob(pattern))
    return out


def bridge_pairs():
    """What the bridge table says: an Agora name, and what calls it here."""
    pairs = {}
    table = root / 'docs' / 'agora-bridge.md'
    if not table.exists():
        return pairs
    row = re.compile(r'^\|\s*`([^`]+)`\s*\|\s*`([^`]+)`\s*\|')
    for line in table.read_text().splitlines():
        if matched := row.match(line):
            agora, here = matched.groups()
            pairs[agora] = [one.strip() for one in here.split(',')]
    return pairs


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--missing', action='store_true',
                        help='list the names not found')
    parser.add_argument('--min', type=int, default=0,
                        help='fail under this many found')
    args = parser.parse_args()

    names = sorted({name for entry in json.loads(
        (root / 'tools' / 'agora-map.json').read_text())
        for name in entry['entities']})
    texts = [path.read_text(errors='ignore') for path in source_files()]
    bridge = bridge_pairs()

    def stands(word):
        asked = re.compile(r'\b' + re.escape(word) + r'\b')
        return any(asked.search(text) for text in texts)

    found, missing = [], []
    for name in names:
        if stands(name) or any(stands(here) for here in bridge.get(name, [])):
            found.append(name)
        else:
            missing.append(name)

    print(f'{len(found)} of {len(names)} names of Agora stand here '
          f'({len(found) * 100 // len(names)}%), '
          f'{len(bridge)} of them through the bridge')
    if args.missing:
        for name in missing:
            print(f'  missing  {name}')
    if len(found) < args.min:
        print(f'under the floor of {args.min}: a carried thing went missing, '
              f'or a rename needs the bridge')
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
