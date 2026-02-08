#!/usr/bin/env python3
"""
Upgrade/normalize Tiled map metadata for framework2D.

What this script does:
1) Adds missing tile-layer metadata fields used by the engine:
   - startx, starty, x, y, width, height, visible
2) Adds/updates tile-layer collision_mode property:
   - solid | one_way | none
3) Normalizes unnamed object shapes (map object layers + tileset objectgroups):
   - square, rectangle, polygon, circle, polyline
4) Optional: annotate tileset tile entries with collision_shape metadata.

Design intent:
- Handle older/incomplete TMJ files exported by Tiled and transliterate them
  into the newer tagging format (testMap.tmj-style metadata).
- Prefer deterministic behavior and conservative defaults.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from collections import Counter
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple


MODE_NONE = "none"
MODE_SOLID = "solid"
MODE_ONE_WAY = "one_way"
VALID_MODES = {MODE_NONE, MODE_SOLID, MODE_ONE_WAY}


def _load_json(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as fh:
        return json.load(fh)


def _save_json(path: Path, data: dict) -> None:
    with path.open("w", encoding="utf-8") as fh:
        json.dump(data, fh, indent=1, ensure_ascii=True)
        fh.write("\n")


def _norm_mode(value: str) -> Optional[str]:
    raw = (value or "").strip().lower()
    if raw in VALID_MODES:
        return raw
    if raw in {"oneway", "one-way"}:
        return MODE_ONE_WAY
    return None


def _get_prop(props: List[dict], name: str) -> Optional[dict]:
    for prop in props:
        if str(prop.get("name", "")).lower() == name.lower():
            return prop
    return None


def _set_prop(props: List[dict], name: str, value, type_name: str = "string") -> None:
    target = _get_prop(props, name)
    if target is None:
        props.append({"name": name, "type": type_name, "value": value})
    else:
        target["type"] = type_name
        target["value"] = value


def _shape_from_object(obj: dict) -> str:
    obj_type = str(obj.get("type", "")).strip().lower()
    if "polygon" in obj:
        return "polygon"
    if obj.get("ellipse", False) or obj_type == "circle":
        return "circle"
    if "polyline" in obj:
        return "polyline"

    w = float(obj.get("width", 0.0) or 0.0)
    h = float(obj.get("height", 0.0) or 0.0)
    if w > 0.0 and h > 0.0:
        if obj_type == "square":
            return "square"
        if abs(w - h) <= 1e-3:
            return "square"
        return "rectangle"

    return "unknown"


def _normalize_object_shape_type(obj: dict) -> Optional[str]:
    inferred = _shape_from_object(obj)
    if inferred == "unknown":
        return None
    if not str(obj.get("type", "")).strip():
        obj["type"] = inferred
    return inferred


def _layer_gids(layer: dict) -> List[int]:
    gids: List[int] = []
    if isinstance(layer.get("data"), list):
        gids.extend(int(v) for v in layer["data"] if int(v) > 0)
    if isinstance(layer.get("chunks"), list):
        for chunk in layer["chunks"]:
            data = chunk.get("data", [])
            gids.extend(int(v) for v in data if int(v) > 0)
    return gids


def _infer_mode_from_name(name: str) -> Optional[str]:
    lname = (name or "").strip().lower()
    if not lname:
        return None

    none_keywords = (
        "bg",
        "background",
        "foreground",
        "fg",
        "deco",
        "decor",
        "parallax",
        "visual",
    )
    one_way_keywords = (
        "platform",
        "oneway",
        "one_way",
        "one-way",
        "semi",
        "item",
    )
    solid_keywords = (
        "main",
        "ground",
        "terrain",
        "collision",
        "solid",
        "wall",
        "floor",
    )

    if any(k in lname for k in one_way_keywords):
        return MODE_ONE_WAY
    if any(k in lname for k in solid_keywords):
        return MODE_SOLID
    if any(k in lname for k in none_keywords):
        return MODE_NONE
    return None


def _build_tsj_collision_index(tsj: dict, first_gid: int) -> Dict[int, dict]:
    tile_h = int(tsj.get("tileheight", 0) or 0)
    index: Dict[int, dict] = {}

    for tile in tsj.get("tiles", []) or []:
        tile_id = int(tile.get("id", -1))
        if tile_id < 0:
            continue
        gid = first_gid + tile_id

        objects = (((tile.get("objectgroup") or {}).get("objects")) or [])
        shape_counter: Counter = Counter()
        top_strip_like = True
        has_rect_like = False

        for obj in objects:
            shape = _shape_from_object(obj)
            shape_counter[shape] += 1
            if shape in {"square", "rectangle"}:
                has_rect_like = True
                h = float(obj.get("height", 0.0) or 0.0)
                y = float(obj.get("y", 0.0) or 0.0)
                # Top-strip heuristic for one-way collision geometry.
                if not (tile_h > 0 and h <= tile_h * 0.5 and y >= tile_h * 0.4):
                    top_strip_like = False
            else:
                top_strip_like = False

        if not has_rect_like and shape_counter:
            top_strip_like = False

        index[gid] = {
            "shape_counter": shape_counter,
            "has_collision": bool(shape_counter and not (len(shape_counter) == 1 and "unknown" in shape_counter)),
            "top_strip_like": bool(shape_counter) and top_strip_like,
        }

    return index


def _dominant_shape(counter: Counter) -> str:
    if not counter:
        return "none"
    pairs = sorted(counter.items(), key=lambda p: (-p[1], p[0]))
    return pairs[0][0]


def _annotate_tileset_shapes(tsj: dict) -> Tuple[int, Counter, Counter]:
    updated = 0
    summary: Counter = Counter()
    mode_summary: Counter = Counter()
    tile_h = int(tsj.get("tileheight", 0) or 0)

    for tile in tsj.get("tiles", []) or []:
        objects = (((tile.get("objectgroup") or {}).get("objects")) or [])
        if not objects:
            continue

        shape_counter: Counter = Counter()
        top_strip_like = True
        has_rect_like = False
        for obj in objects:
            inferred = _normalize_object_shape_type(obj)
            if inferred:
                shape_counter[inferred] += 1
                if inferred in {"square", "rectangle"}:
                    has_rect_like = True
                    h = float(obj.get("height", 0.0) or 0.0)
                    y = float(obj.get("y", 0.0) or 0.0)
                    if not (tile_h > 0 and h <= tile_h * 0.5 and y >= tile_h * 0.4):
                        top_strip_like = False
                else:
                    top_strip_like = False

        if not shape_counter:
            continue

        if not has_rect_like:
            top_strip_like = False

        dominant = _dominant_shape(shape_counter)
        summary[dominant] += 1

        if top_strip_like:
            mode_hint = MODE_ONE_WAY
        else:
            mode_hint = MODE_SOLID
        mode_summary[mode_hint] += 1

        props = tile.setdefault("properties", [])
        if not isinstance(props, list):
            props = []
            tile["properties"] = props

        _set_prop(props, "collision_shape", dominant, "string")
        _set_prop(props, "collision_shapes", ",".join(sorted(shape_counter.keys())), "string")
        _set_prop(props, "collision_object_count", int(sum(shape_counter.values())), "int")
        _set_prop(props, "collision_mode_hint", mode_hint, "string")
        updated += 1

    return updated, summary, mode_summary


def _layer_dimensions(layer: dict, fallback_w: int, fallback_h: int) -> Tuple[int, int]:
    width = int(layer.get("width", fallback_w) or fallback_w)
    height = int(layer.get("height", fallback_h) or fallback_h)
    if width > 0 and height > 0:
        return width, height

    if isinstance(layer.get("chunks"), list) and layer["chunks"]:
        # Use layer's own width/height if available; fallback to map size otherwise.
        return max(width, fallback_w), max(height, fallback_h)

    if isinstance(layer.get("data"), list) and fallback_w > 0:
        inferred_h = len(layer["data"]) // fallback_w
        return fallback_w, inferred_h if inferred_h > 0 else fallback_h

    return max(width, fallback_w), max(height, fallback_h)


def _infer_collision_mode(
    layer: dict,
    collision_index: Dict[int, dict],
    default_mode: str,
) -> Tuple[str, str]:
    by_name = _infer_mode_from_name(str(layer.get("name", "")))
    if by_name:
        return by_name, "layer-name"

    gids = _layer_gids(layer)
    if not gids:
        return MODE_NONE, "empty-layer"

    if not collision_index:
        return default_mode, "default-no-tileset"

    collidable = 0
    top_strip_like = 0
    unknown = 0
    for gid in gids:
        info = collision_index.get(gid)
        if not info:
            unknown += 1
            continue
        if info["has_collision"]:
            collidable += 1
            if info["top_strip_like"]:
                top_strip_like += 1

    if collidable == 0:
        return MODE_NONE, "no-collidable-tiles"
    if top_strip_like >= max(1, int(collidable * 0.8)):
        return MODE_ONE_WAY, "tileset-top-strip-heuristic"
    return MODE_SOLID, "tileset-collision-heuristic"


def upgrade_map(
    tmj_path: Path,
    tsj_path: Optional[Path],
    write: bool,
    annotate_tileset: bool,
    force_mode: bool,
    default_mode: str,
) -> int:
    tmj = _load_json(tmj_path)
    if tmj.get("type") != "map":
        print(f"error: {tmj_path} is not a Tiled map", file=sys.stderr)
        return 2

    map_w = int(tmj.get("width", 0) or 0)
    map_h = int(tmj.get("height", 0) or 0)

    # Resolve tileset source from map if explicit path not provided.
    first_gid = 1
    if tsj_path is None:
        for ts in tmj.get("tilesets", []) or []:
            src = ts.get("source")
            if isinstance(src, str) and src:
                first_gid = int(ts.get("firstgid", 1) or 1)
                tsj_path = (tmj_path.parent / src).resolve()
                break

    collision_index: Dict[int, dict] = {}
    tsj = None
    if tsj_path and tsj_path.exists():
        tsj = _load_json(tsj_path)
        collision_index = _build_tsj_collision_index(tsj, first_gid)

    changed = False
    mode_report: List[Tuple[int, str, str, str]] = []

    for layer in tmj.get("layers", []) or []:
        if layer.get("type") == "tilelayer":
            # Normalize expected metadata fields.
            if "startx" not in layer:
                layer["startx"] = 0
                changed = True
            if "starty" not in layer:
                layer["starty"] = 0
                changed = True
            if "x" not in layer:
                layer["x"] = 0
                changed = True
            if "y" not in layer:
                layer["y"] = 0
                changed = True
            if "visible" not in layer:
                layer["visible"] = True
                changed = True

            lw, lh = _layer_dimensions(layer, map_w, map_h)
            if int(layer.get("width", 0) or 0) != lw:
                layer["width"] = lw
                changed = True
            if int(layer.get("height", 0) or 0) != lh:
                layer["height"] = lh
                changed = True

            props = layer.setdefault("properties", [])
            if not isinstance(props, list):
                props = []
                layer["properties"] = props
                changed = True

            mode_prop = _get_prop(props, "collision_mode")
            current_mode = None
            if mode_prop is not None:
                current_mode = _norm_mode(str(mode_prop.get("value", "")))

            if force_mode or current_mode is None:
                inferred, reason = _infer_collision_mode(layer, collision_index, default_mode)
                _set_prop(props, "collision_mode", inferred, "string")
                mode_report.append((int(layer.get("id", -1)), str(layer.get("name", "")), inferred, reason))
                if current_mode != inferred:
                    changed = True
            else:
                mode_report.append((int(layer.get("id", -1)), str(layer.get("name", "")), current_mode, "preserved"))

        elif layer.get("type") == "objectgroup":
            for obj in layer.get("objects", []) or []:
                inferred = _normalize_object_shape_type(obj)
                if inferred and str(obj.get("type", "")).strip().lower() == inferred:
                    # If we populated type for an empty source, this is a mutation.
                    changed = True

    tile_annotated = 0
    tile_shape_summary = Counter()
    tile_mode_summary = Counter()
    if tsj and annotate_tileset:
        tile_annotated, tile_shape_summary, tile_mode_summary = _annotate_tileset_shapes(tsj)
        if tile_annotated > 0:
            changed = True

    print(f"Map: {tmj_path}")
    if tsj_path:
        print(f"Tileset: {tsj_path}")
    print("Layer collision_mode decisions:")
    for layer_id, name, mode, reason in mode_report:
        print(f"  - id={layer_id:<3} name='{name}' mode={mode:<7} ({reason})")

    if tsj and annotate_tileset:
        print(f"Tileset shape tags updated: {tile_annotated}")
        if tile_shape_summary:
            counts = ", ".join(f"{k}={v}" for k, v in sorted(tile_shape_summary.items()))
            print(f"  dominant shapes: {counts}")
        if tile_mode_summary:
            counts = ", ".join(f"{k}={v}" for k, v in sorted(tile_mode_summary.items()))
            print(f"  collision_mode_hint: {counts}")

    if write and changed:
        _save_json(tmj_path, tmj)
        print(f"wrote: {tmj_path}")
        if tsj and annotate_tileset:
            _save_json(tsj_path, tsj)
            print(f"wrote: {tsj_path}")
    elif write:
        print("no changes needed")
    else:
        print("dry-run (no files written)")

    return 0


def _parse_args(argv: Sequence[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Upgrade Tiled TMJ metadata for framework2D.")
    parser.add_argument("--map", required=True, type=Path, help="Path to input .tmj map")
    parser.add_argument(
        "--tileset",
        type=Path,
        default=None,
        help="Optional .tsj path (defaults to first map tileset source)",
    )
    parser.add_argument("--write", action="store_true", help="Write changes in-place")
    parser.add_argument(
        "--annotate-tileset",
        action="store_true",
        help="Tag tileset tile collision shapes into tile properties",
    )
    parser.add_argument(
        "--force-mode",
        action="store_true",
        help="Overwrite existing collision_mode properties",
    )
    parser.add_argument(
        "--default-mode",
        default=MODE_NONE,
        choices=sorted(VALID_MODES),
        help="Fallback mode when inference is ambiguous",
    )
    return parser.parse_args(argv)


def main(argv: Sequence[str]) -> int:
    args = _parse_args(argv)
    map_path = args.map.resolve()
    if not map_path.exists():
        print(f"error: map not found: {map_path}", file=sys.stderr)
        return 2

    tileset_path = args.tileset.resolve() if args.tileset else None
    return upgrade_map(
        tmj_path=map_path,
        tsj_path=tileset_path,
        write=args.write,
        annotate_tileset=args.annotate_tileset,
        force_mode=args.force_mode,
        default_mode=args.default_mode,
    )


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
