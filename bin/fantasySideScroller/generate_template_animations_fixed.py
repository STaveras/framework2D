#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
generate_template_animations.py

Derive Animations.json from a sprite-sheet template folder using heuristics:

- Each *subdirectory* under the target directory becomes an animation "state".
  The state name is the directory name with any parenthesized suffix removed
  (e.g. "Idle (Sword)" -> "Idle"). Directories whose *entire* name is in
  parentheses (e.g. "(Test)") are ignored.

- For each state directory, the first .png found (sorted by name) is used as the
  sprite sheet.

- Frame dimensions are parsed from a "<w>x<h>" suffix in the PNG filename,
  e.g. "Player Idle 48x48.png" -> frame_w=48, frame_h=48.

- The number of frames is derived from the sheet's pixel dimensions. We compute
  the horizontal frame count as sheet_width // frame_w. If the sheet contains
  multiple rows, we also compute vertical frame count as sheet_height // frame_h
  and generate frames row-major (left-to-right, top-to-bottom).

- Output JSON conforms to the schema of the provided example Animations.json:
  {
    "AnimationFile": {
      "Animation": [
        {
          "Name": "...",
          "Mode": "...",
          "Forward": true,
          "Speed": 1,
          "Frame": [
            {"DisplayRect": "x,y,w,h", "Filename": "./path.png", "Duration": 0.133333},
            ...
          ]
        },
        ...
      ]
    }
  }

Notes / defaults:
- Mode: "LOOPING"
- Forward: true
- Speed: 1
- Duration per frame: 0.133333 (7.5 FPS-ish) — adjust via CLI if desired.
"""

from __future__ import annotations

import argparse
import json
import re
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable, Tuple


DIM_RE = re.compile(r"(?P<w>\d+)\s*[xX]\s*(?P<h>\d+)(?=\.png$)", re.IGNORECASE)
PARENS_RE = re.compile(r"\s*\(.*?\)\s*")


@dataclass(frozen=True)
class SheetInfo:
    sheet_path: Path
    frame_w: int
    frame_h: int
    sheet_w: int
    sheet_h: int


def _clean_state_name(dir_name: str) -> str:
    # Remove anything in parentheses anywhere in the name.
    cleaned = PARENS_RE.sub("", dir_name).strip()
    return cleaned


def _iter_state_dirs(root: Path) -> Iterable[Path]:
    for p in sorted(root.iterdir(), key=lambda x: x.name.lower()):
        if not p.is_dir():
            continue
        # Ignore directories whose entire name is parenthesized (e.g. "(Test)").
        if p.name.strip().startswith("(") and p.name.strip().endswith(")"):
            continue
        if p.name.startswith("."):
            continue
        yield p


def _first_png(state_dir: Path) -> Path | None:
    pngs = sorted(state_dir.glob("*.png"), key=lambda x: x.name.lower())
    return pngs[0] if pngs else None


def _parse_frame_dims(png_path: Path) -> Tuple[int, int] | None:
    m = DIM_RE.search(png_path.name)
    if not m:
        # Fallback: find *any* "<w>x<h>" occurrence (not necessarily suffix)
        m2 = re.search(r"(\d+)\s*[xX]\s*(\d+)", png_path.stem)
        if not m2:
            return None
        return int(m2.group(1)), int(m2.group(2))
    return int(m.group("w")), int(m.group("h"))


def _read_png_wh(png_path: Path) -> Tuple[int, int]:
    """
    Read width/height from PNG IHDR without Pillow.
    PNG header:
      - signature: 8 bytes
      - IHDR chunk: length(4) type(4) data(13) crc(4)
      - width at offset 16..19, height at offset 20..23
    """
    with png_path.open("rb") as f:
        f.seek(16)
        wh = f.read(8)
        if len(wh) != 8:
            raise ValueError("Incomplete PNG header (IHDR).")
        w = int.from_bytes(wh[0:4], "big")
        h = int.from_bytes(wh[4:8], "big")
        return w, h


def _sheet_info(state_dir: Path) -> SheetInfo | None:
    sheet_path = _first_png(state_dir)
    if not sheet_path:
        return None
    dims = _parse_frame_dims(sheet_path)
    if not dims:
        return None
    frame_w, frame_h = dims
    if frame_w <= 0 or frame_h <= 0:
        return None
    sheet_w, sheet_h = _read_png_wh(sheet_path)
    return SheetInfo(sheet_path=sheet_path, frame_w=frame_w, frame_h=frame_h, sheet_w=sheet_w, sheet_h=sheet_h)


def _to_json_relpath(path: Path, base: Path) -> str:
    # Use "./" + POSIX-style relative path to match the example.
    rel = path.relative_to(base).as_posix()
    return f"./{rel}"


def _build_frames(info: SheetInfo, base: Path, duration: float) -> list[dict]:
    cols = max(1, info.sheet_w // info.frame_w)
    rows = max(1, info.sheet_h // info.frame_h)
    total = cols * rows

    filename = _to_json_relpath(info.sheet_path, base)

    frames: list[dict] = []
    for i in range(total):
        x = (i % cols) * info.frame_w
        y = (i // cols) * info.frame_h
        frames.append(
            {
                "DisplayRect": f"{x},{y},{info.frame_w},{info.frame_h}",
                "Filename": filename,
                "Duration": float(duration),
            }
        )
    return frames


def generate(target_dir: Path, out_file: Path, *, duration: float, default_mode: str, default_speed: float, forward: bool) -> dict:
    animations: list[dict] = []

    for state_dir in _iter_state_dirs(target_dir):
        state_name = _clean_state_name(state_dir.name)
        if not state_name:
            continue

        info = _sheet_info(state_dir)
        if not info:
            # Skip silently; caller can enable verbose in the future if needed.
            continue

        anim = {
            "Name": state_name,
            "Mode": default_mode,
            "Forward": bool(forward),
            "Speed": float(default_speed),
            "Frame": _build_frames(info, base=out_file.parent, duration=duration),
        }
        animations.append(anim)

    return {"AnimationFile": {"Animation": animations}}


def main() -> int:
    ap = argparse.ArgumentParser(description="Generate Animations.json from a template sprite-sheet folder.")
    ap.add_argument(
        "--target",
        default="2D-Pixel-Art-Character-Template",
        help="Target template directory (contains state subfolders). Default: %(default)s",
    )
    ap.add_argument(
        "--out",
        default="Animations.json",
        help="Output JSON file path. Default: %(default)s",
    )
    ap.add_argument(
        "--duration",
        type=float,
        default=0.133333,
        help="Per-frame duration in seconds. Default: %(default)s",
    )
    ap.add_argument(
        "--mode",
        default="LOOPING",
        help='Default animation mode string (e.g. LOOPING/OSCILLATE/ONCE). Default: %(default)s',
    )
    ap.add_argument(
        "--speed",
        type=float,
        default=1.0,
        help="Default animation speed scalar. Default: %(default)s",
    )
    ap.add_argument(
        "--forward",
        action="store_true",
        default=True,
        help="Write Forward=true (default). Use --no-forward to write false.",
    )
    ap.add_argument(
        "--no-forward",
        dest="forward",
        action="store_false",
        help="Write Forward=false.",
    )

    args = ap.parse_args()
    cwd = Path.cwd()

    target_dir = (cwd / args.target).resolve()
    out_file = (cwd / args.out).resolve()

    if not target_dir.exists() or not target_dir.is_dir():
        raise SystemExit(f"Target directory not found or not a directory: {target_dir}")

    out_file.parent.mkdir(parents=True, exist_ok=True)

    data = generate(
        target_dir=target_dir,
        out_file=out_file,
        duration=args.duration,
        default_mode=args.mode,
        default_speed=args.speed,
        forward=args.forward,
    )

    out_file.write_text(json.dumps(data, indent=2), encoding="utf-8")
    print(f"Wrote {out_file} with {len(data['AnimationFile']['Animation'])} animations.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
