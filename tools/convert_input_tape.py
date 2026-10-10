#!/usr/bin/env python3
"""Convert an old input tape to the current format.

Old tapes have a "tick,action,event_type,active" header and up to four rows
per action per tick. Current tapes have a "tick,action,down" header and one
row each time an action changes (see src/InputTapeRecorder.h).

Usage: tools/convert_input_tape.py <old.csv> <new.csv>
"""

import csv
import sys


def convert(old_path, new_path):
    rows = []
    with open(old_path, newline="") as f:
        for sequence, row in enumerate(csv.reader(f)):
            if len(row) < 4 or not row[0].strip().isdigit():
                continue  # header or malformed
            tick, action, active = int(row[0]), row[1].strip(), row[3].strip() == "1"
            rows.append((tick, sequence, action, active))
    # The old replayer applied every row up to a tick, in tick order, so only
    # each action's last row within a tick counts.
    rows.sort()
    final = {}
    for tick, _, action, active in rows:
        final[(tick, action)] = active

    state = {}
    changes = 0
    with open(new_path, "w", newline="") as f:
        out = csv.writer(f, lineterminator="\n")
        out.writerow(["tick", "action", "down"])
        for (tick, action), down in final.items():
            if state.get(action, False) != down:
                out.writerow([tick, action, 1 if down else 0])
                state[action] = down
                changes += 1
    print(f"{old_path}: {len(rows)} rows -> {new_path}: {changes} changes")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        sys.exit(__doc__)
    convert(sys.argv[1], sys.argv[2])
