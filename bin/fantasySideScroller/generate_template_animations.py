#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""Generate animation JSON from sprite sheets.

The script reads an existing ``Animations.json`` file, then walks the
``2D-Pixel-Art-Character-Template`` directory.  For each state folder it:

1. Finds the first PNG file (the sprite sheet).
2. Parses the frame width from a dimension suffix in the filename
   (e.g., ``48x48`` → 48px).
3. Opens the image with Pillow to get its total width and calculates how many
   frames fit horizontally.
4. Builds a list of ``Frame`` objects pointing back to that PNG file.

The resulting JSON is written back to ``Animations.json``.
"""

import json
import re
from pathlib import Path


def main() -> None:
    cwd = Path.cwd()
    dst_dir = cwd / "2D-Pixel-Art-Character-Template"
    src_file = cwd / "Animations.json"

    if not src_file.exists():
        print(f"Source file missing: {src_file}")
        return

    data = json.loads(src_file.read_text(encoding="utf-8"))

    # Iterate over each animation definition and populate frame data
    for anim in data["AnimationFile"]["Animation"]:
        name = anim["Name"]

        # Find the corresponding directory under dst_dir.  Ignore any
        # sub‑directories that contain parentheses (e.g., "(Test)").
        state_dirs = [p for p in dst_dir.iterdir() if p.is_dir() and not p.name.startswith("(")]
        target_dir = next((d for d in state_dirs if d.name.lower() == name.lower()), None)
        if not target_dir:
            print(f"Warning: No directory found for animation '{name}'. Skipping.")
            continue

        # Locate the first PNG file in that directory – this is assumed to be
        # the sprite sheet for the animation.
        png_files = list(target_dir.glob("*.png"))
        if not png_files:
            print(f"Warning: No PNG found in {target_dir}. Skipping.")
            continue
        sheet_path = png_files[0]

        # Extract frame width from the filename.  The convention is that the
        # filename contains a dimension like "48x48" before the extension.
        match = re.search(r"(\d+)x\d+", sheet_path.name)
        if not match:
            print(f"Warning: Could not parse frame dimensions from {sheet_path}. Skipping.")
            continue
        frame_width = int(match.group(1))

        # Determine the total number of frames by inspecting the image width.
        # Read PNG header to get width without Pillow.  The IHDR chunk starts
        # at byte 8 and contains the width in bytes 16‑19 (big endian).
        try:
            with sheet_path.open("rb") as f:
                f.seek(16)
                width_bytes = f.read(4)
                if len(width_bytes) != 4:
                    raise ValueError("Incomplete PNG header")
                sheet_width = int.from_bytes(width_bytes, "big")
        except Exception as e:
            print(f"Warning: Unable to read image {sheet_path}: {e}. Skipping.")
            continue

        frame_count = max(1, sheet_width // frame_width)

        # Build the Frame list for this animation.  Each frame will reference
        # the same sprite sheet; additional metadata can be added if needed.
        frames = []
        tmpl_path = str(sheet_path.relative_to(cwd))
        for _ in range(frame_count):
            frames.append({"Filename": tmpl_path})
        anim["Frame"] = frames

    src_file.write_text(json.dumps(data, indent=4), encoding="utf-8")


if __name__ == "__main__":
    main()
