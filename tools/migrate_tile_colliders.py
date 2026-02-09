#!/usr/bin/env python3
"""
Populate explicit full-tile collision objectgroups for tiles used by colliding layers,
and optionally normalize layer collision modes + prune decorative auto colliders.

Usage:
  python3 tools/migrate_tile_colliders.py --maps map1.tmj map2.tmj --report report.json
  python3 tools/migrate_tile_colliders.py --maps map.tmj --write --backup
  python3 tools/migrate_tile_colliders.py --maps map.tmj --set-layer-mode "tiles=solid,props=none" --prune-auto --write
"""

from __future__ import annotations

import argparse
import json
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Dict, List, Optional, Set, Tuple


GID_MASK = 0x1FFFFFFF


@dataclass
class TileSetUsage:
    path: Path
    firstgid: int
    used_tile_ids: Set[int] = field(default_factory=set)


@dataclass
class MapTileSetReport:
    path: str
    used_tile_ids: int = 0


@dataclass
class LayerModeUpdate:
    layer_id: int
    layer_name: str
    from_mode: str
    to_mode: str


@dataclass
class TileSetReport:
    path: str
    used_tile_ids: int = 0
    existing_colliders: int = 0
    added_colliders: int = 0
    auto_colliders_pruned: int = 0
    auto_colliders_kept: int = 0
    manual_colliders_kept: int = 0
    skipped_missing_tileset: bool = False


@dataclass
class MapReport:
    path: str
    colliding_layers: int = 0
    layer_mode_updates: List[LayerModeUpdate] = field(default_factory=list)
    tilesets: List[MapTileSetReport] = field(default_factory=list)
    warnings: List[str] = field(default_factory=list)


@dataclass
class MigrationReport:
    maps: List[MapReport] = field(default_factory=list)
    tilesets: List[TileSetReport] = field(default_factory=list)
    written_files: List[str] = field(default_factory=list)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Migrate map-driven tile collision shapes to explicit tileset objectgroups")
    parser.add_argument("--maps", nargs="+", required=True, help="TMJ map files to inspect")
    parser.add_argument("--write", action="store_true", help="Write changes to map/tileset files")
    parser.add_argument("--backup", action="store_true", help="Write .bak backups for modified files")
    parser.add_argument("--report", default="", help="Optional JSON report output path")
    parser.add_argument(
        "--set-layer-mode",
        default="",
        help=(
            "Comma-separated collision mode policy for tile layer names, e.g. "
            "'tiles=solid,props=none,details=none,objects=none,items=none'"
        ),
    )
    parser.add_argument(
        "--prune-auto",
        action="store_true",
        help="Remove auto_full_tile colliders from tiles not used by colliding layers",
    )
    return parser.parse_args()


def canonical_collision_mode(value: Any) -> str:
    normalized = str(value).strip().lower()
    if normalized == "solid":
        return "solid"
    if normalized in ("one_way", "oneway"):
        return "one_way"
    return "none"


def parse_layer_mode_overrides(raw_value: str) -> Dict[str, str]:
    overrides: Dict[str, str] = {}
    raw_value = raw_value.strip()
    if not raw_value:
        return overrides

    for token in raw_value.split(","):
        entry = token.strip()
        if not entry:
            continue
        if "=" not in entry:
            raise ValueError(f"Invalid --set-layer-mode entry '{entry}' (expected name=mode)")

        raw_name, raw_mode = entry.split("=", 1)
        layer_name = raw_name.strip().lower()
        mode_token = raw_mode.strip().lower()
        if not layer_name:
            raise ValueError(f"Invalid --set-layer-mode entry '{entry}' (missing layer name)")
        if mode_token not in ("none", "solid", "one_way", "oneway"):
            raise ValueError(
                f"Invalid collision mode '{raw_mode}' for layer '{raw_name}'. "
                "Allowed: none, solid, one_way"
            )

        overrides[layer_name] = canonical_collision_mode(mode_token)

    return overrides


def normalize_collision_mode(layer: Dict[str, Any]) -> str:
    for prop in layer.get("properties", []):
        if not isinstance(prop, dict):
            continue
        if str(prop.get("name", "")).lower() != "collision_mode":
            continue
        return canonical_collision_mode(prop.get("value", ""))
    return "none"


def ensure_properties_list(layer: Dict[str, Any]) -> List[Dict[str, Any]]:
    properties = layer.get("properties")
    if not isinstance(properties, list):
        properties = []
        layer["properties"] = properties
    return properties


def apply_layer_mode_policy(map_json: Dict[str, Any], policy: Dict[str, str]) -> List[LayerModeUpdate]:
    updates: List[LayerModeUpdate] = []
    if not policy:
        return updates

    layers = map_json.get("layers")
    if not isinstance(layers, list):
        return updates

    for layer in layers:
        if not isinstance(layer, dict) or layer.get("type") != "tilelayer":
            continue

        layer_name = str(layer.get("name", ""))
        policy_key = layer_name.lower()
        if policy_key not in policy:
            continue

        desired_mode = policy[policy_key]
        previous_mode = normalize_collision_mode(layer)

        properties = ensure_properties_list(layer)
        collision_prop: Optional[Dict[str, Any]] = None
        for prop in properties:
            if isinstance(prop, dict) and str(prop.get("name", "")).lower() == "collision_mode":
                collision_prop = prop
                break

        changed = False
        if collision_prop is None:
            properties.append({"name": "collision_mode", "type": "string", "value": desired_mode})
            changed = True
        else:
            if collision_prop.get("type") != "string":
                collision_prop["type"] = "string"
                changed = True

            current_value = canonical_collision_mode(collision_prop.get("value", ""))
            if current_value != desired_mode or str(collision_prop.get("value", "")) != desired_mode:
                collision_prop["value"] = desired_mode
                changed = True

        if changed:
            updates.append(
                LayerModeUpdate(
                    layer_id=int(layer.get("id", -1)),
                    layer_name=layer_name,
                    from_mode=previous_mode,
                    to_mode=desired_mode,
                )
            )

    return updates


def normalize_gid(raw_gid: int) -> int:
    return int(raw_gid) & GID_MASK


def gather_colliding_gid_usage(map_json: Dict[str, Any]) -> Set[int]:
    gids: Set[int] = set()
    for layer in map_json.get("layers", []):
        if not isinstance(layer, dict) or layer.get("type") != "tilelayer":
            continue

        mode = normalize_collision_mode(layer)
        if mode == "none":
            continue

        data = layer.get("data")
        if isinstance(data, list):
            for gid in data:
                if isinstance(gid, int):
                    ngid = normalize_gid(gid)
                    if ngid > 0:
                        gids.add(ngid)

        chunks = layer.get("chunks")
        if isinstance(chunks, list):
            for chunk in chunks:
                if not isinstance(chunk, dict):
                    continue
                chunk_data = chunk.get("data")
                if not isinstance(chunk_data, list):
                    continue
                for gid in chunk_data:
                    if isinstance(gid, int):
                        ngid = normalize_gid(gid)
                        if ngid > 0:
                            gids.add(ngid)

    return gids


def count_colliding_layers(map_json: Dict[str, Any]) -> int:
    count = 0
    for layer in map_json.get("layers", []):
        if not isinstance(layer, dict) or layer.get("type") != "tilelayer":
            continue
        if normalize_collision_mode(layer) != "none":
            count += 1
    return count


def resolve_tileset_usage(map_path: Path, map_json: Dict[str, Any], colliding_gids: Set[int]) -> List[TileSetUsage]:
    entries: List[Tuple[int, Path]] = []

    for tileset_ref in map_json.get("tilesets", []):
        if not isinstance(tileset_ref, dict):
            continue

        firstgid = int(tileset_ref.get("firstgid", 0))
        source = tileset_ref.get("source")
        if firstgid <= 0 or not isinstance(source, str) or not source:
            continue

        resolved = (map_path.parent / source).resolve()
        entries.append((firstgid, resolved))

    entries.sort(key=lambda t: t[0])

    usages: List[TileSetUsage] = []
    for firstgid, path in entries:
        usages.append(TileSetUsage(path=path, firstgid=firstgid))

    if not usages:
        return usages

    for gid in colliding_gids:
        selected: Optional[TileSetUsage] = None
        for usage in reversed(usages):
            if gid >= usage.firstgid:
                selected = usage
                break
        if not selected:
            continue

        selected.used_tile_ids.add(gid - selected.firstgid)

    return usages


def is_collision_object(obj: Any) -> bool:
    if not isinstance(obj, dict):
        return False
    if isinstance(obj.get("polygon"), list):
        return True
    return obj.get("width") is not None and obj.get("height") is not None


def is_auto_full_tile_object(obj: Any) -> bool:
    return isinstance(obj, dict) and str(obj.get("name", "")).strip() == "auto_full_tile"


def has_collision_object(tile_obj: Dict[str, Any]) -> bool:
    objectgroup = tile_obj.get("objectgroup")
    if not isinstance(objectgroup, dict):
        return False

    objects = objectgroup.get("objects", [])
    if not isinstance(objects, list):
        return False

    return any(is_collision_object(obj) for obj in objects)


def next_object_id(objectgroup: Dict[str, Any]) -> int:
    max_id = 0
    objects = objectgroup.get("objects", [])
    if isinstance(objects, list):
        for obj in objects:
            if isinstance(obj, dict) and isinstance(obj.get("id"), int):
                max_id = max(max_id, int(obj["id"]))
    return max_id + 1


def ensure_objectgroup(tile_obj: Dict[str, Any]) -> Dict[str, Any]:
    objectgroup = tile_obj.get("objectgroup")
    if not isinstance(objectgroup, dict):
        objectgroup = {
            "draworder": "index",
            "id": 0,
            "name": "",
            "objects": [],
            "opacity": 1,
            "type": "objectgroup",
            "visible": True,
            "x": 0,
            "y": 0,
        }
        tile_obj["objectgroup"] = objectgroup

    if not isinstance(objectgroup.get("objects"), list):
        objectgroup["objects"] = []

    return objectgroup


def inject_full_tile_collider(tile_obj: Dict[str, Any], tile_width: int, tile_height: int) -> None:
    objectgroup = ensure_objectgroup(tile_obj)
    object_id = next_object_id(objectgroup)
    objectgroup["objects"].append(
        {
            "height": tile_height,
            "id": object_id,
            "name": "auto_full_tile",
            "rotation": 0,
            "type": "square",
            "visible": True,
            "width": tile_width,
            "x": 0,
            "y": 0,
        }
    )


def migrate_tileset(
    tileset_path: Path,
    used_tile_ids: Set[int],
    prune_auto: bool,
) -> Tuple[Optional[Dict[str, Any]], TileSetReport, bool]:
    report = TileSetReport(path=str(tileset_path), used_tile_ids=len(used_tile_ids))

    if not tileset_path.exists():
        report.skipped_missing_tileset = True
        return None, report, False

    with tileset_path.open("r", encoding="utf-8") as f:
        tileset_json = json.load(f)

    tile_width = int(tileset_json.get("tilewidth", 0))
    tile_height = int(tileset_json.get("tileheight", 0))
    if tile_width <= 0 or tile_height <= 0:
        return tileset_json, report, False

    tiles = tileset_json.get("tiles")
    if not isinstance(tiles, list):
        tiles = []
        tileset_json["tiles"] = tiles

    changed = False

    tile_by_id: Dict[int, Dict[str, Any]] = {}
    for entry in tiles:
        if isinstance(entry, dict) and isinstance(entry.get("id"), int):
            tile_by_id[int(entry["id"])] = entry

    for tile_id in sorted(used_tile_ids):
        if tile_id < 0:
            continue

        tile_obj = tile_by_id.get(tile_id)
        if tile_obj is None:
            tile_obj = {"id": int(tile_id)}
            tiles.append(tile_obj)
            tile_by_id[tile_id] = tile_obj
            changed = True

        if has_collision_object(tile_obj):
            report.existing_colliders += 1
            continue

        inject_full_tile_collider(tile_obj, tile_width, tile_height)
        report.added_colliders += 1
        changed = True

    for entry in tiles:
        if not isinstance(entry, dict):
            continue
        tile_id = entry.get("id")
        if not isinstance(tile_id, int):
            continue

        objectgroup = entry.get("objectgroup")
        if not isinstance(objectgroup, dict):
            continue

        objects = objectgroup.get("objects")
        if not isinstance(objects, list):
            continue

        auto_count = 0
        manual_count = 0
        for obj in objects:
            if not is_collision_object(obj):
                continue
            if is_auto_full_tile_object(obj):
                auto_count += 1
            else:
                manual_count += 1

        report.manual_colliders_kept += manual_count

        if auto_count <= 0:
            continue

        if tile_id in used_tile_ids or not prune_auto:
            report.auto_colliders_kept += auto_count
            continue

        filtered_objects = [
            obj
            for obj in objects
            if not (is_collision_object(obj) and is_auto_full_tile_object(obj))
        ]
        removed_count = len(objects) - len(filtered_objects)
        if removed_count > 0:
            objectgroup["objects"] = filtered_objects
            report.auto_colliders_pruned += removed_count
            changed = True
        else:
            report.auto_colliders_kept += auto_count

    if changed:
        tiles.sort(key=lambda t: int(t.get("id", 0)) if isinstance(t, dict) else 0)

    return tileset_json, report, changed


def write_json(path: Path, payload: Dict[str, Any]) -> None:
    with path.open("w", encoding="utf-8") as f:
        json.dump(payload, f, indent=1, ensure_ascii=False)
        f.write("\n")


def main() -> int:
    args = parse_args()

    try:
        layer_mode_policy = parse_layer_mode_overrides(args.set_layer_mode)
    except ValueError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

    migration_report = MigrationReport()
    pending_writes: Dict[Path, Dict[str, Any]] = {}

    global_tileset_usage: Dict[Path, Set[int]] = {}

    for map_input in args.maps:
        map_path = Path(map_input).resolve()
        map_report = MapReport(path=str(map_path))
        migration_report.maps.append(map_report)

        if not map_path.exists():
            map_report.warnings.append("Map file not found")
            continue

        with map_path.open("r", encoding="utf-8") as f:
            map_json = json.load(f)

        layer_updates = apply_layer_mode_policy(map_json, layer_mode_policy)
        map_report.layer_mode_updates.extend(layer_updates)
        if layer_updates:
            pending_writes[map_path] = map_json

        map_report.colliding_layers = count_colliding_layers(map_json)
        colliding_gids = gather_colliding_gid_usage(map_json)

        usages = resolve_tileset_usage(map_path, map_json, colliding_gids)
        for usage in usages:
            used_count = len(usage.used_tile_ids)
            map_report.tilesets.append(MapTileSetReport(path=str(usage.path), used_tile_ids=used_count))

            if used_count > 0 or args.prune_auto:
                global_tileset_usage.setdefault(usage.path, set()).update(usage.used_tile_ids)

    for tileset_path in sorted(global_tileset_usage.keys(), key=lambda p: str(p)):
        used_ids = global_tileset_usage[tileset_path]
        migrated_json, tileset_report, changed = migrate_tileset(tileset_path, used_ids, args.prune_auto)
        migration_report.tilesets.append(tileset_report)

        if tileset_report.skipped_missing_tileset:
            for map_report in migration_report.maps:
                if any(Path(ts.path) == tileset_path for ts in map_report.tilesets):
                    map_report.warnings.append(f"Missing tileset: {tileset_path}")
            continue

        if migrated_json is not None and changed:
            pending_writes[tileset_path] = migrated_json

    if args.write:
        for path, payload in sorted(pending_writes.items(), key=lambda item: str(item[0])):
            if args.backup:
                backup_path = Path(str(path) + ".bak")
                if not backup_path.exists() and path.exists():
                    backup_path.write_bytes(path.read_bytes())

            write_json(path, payload)
            migration_report.written_files.append(str(path))

    if args.report:
        report_path = Path(args.report).resolve()
        report_payload = {
            "maps": [
                {
                    "path": entry.path,
                    "colliding_layers": entry.colliding_layers,
                    "warnings": entry.warnings,
                    "layer_mode_updates": [
                        {
                            "layer_id": update.layer_id,
                            "layer_name": update.layer_name,
                            "from_mode": update.from_mode,
                            "to_mode": update.to_mode,
                        }
                        for update in entry.layer_mode_updates
                    ],
                    "tilesets": [
                        {
                            "path": ts.path,
                            "used_tile_ids": ts.used_tile_ids,
                        }
                        for ts in entry.tilesets
                    ],
                }
                for entry in migration_report.maps
            ],
            "tilesets": [
                {
                    "path": ts.path,
                    "used_tile_ids": ts.used_tile_ids,
                    "existing_colliders": ts.existing_colliders,
                    "added_colliders": ts.added_colliders,
                    "auto_colliders_pruned": ts.auto_colliders_pruned,
                    "auto_colliders_kept": ts.auto_colliders_kept,
                    "manual_colliders_kept": ts.manual_colliders_kept,
                    "skipped_missing_tileset": ts.skipped_missing_tileset,
                }
                for ts in migration_report.tilesets
            ],
            "written_files": migration_report.written_files,
        }
        write_json(report_path, report_payload)

    total_added = sum(ts.added_colliders for ts in migration_report.tilesets)
    total_existing = sum(ts.existing_colliders for ts in migration_report.tilesets)
    total_pruned = sum(ts.auto_colliders_pruned for ts in migration_report.tilesets)
    total_kept_auto = sum(ts.auto_colliders_kept for ts in migration_report.tilesets)
    total_kept_manual = sum(ts.manual_colliders_kept for ts in migration_report.tilesets)
    total_layer_updates = sum(len(m.layer_mode_updates) for m in migration_report.maps)

    print(
        "Collider migration summary: "
        f"existing={total_existing} "
        f"added={total_added} "
        f"auto_pruned={total_pruned} "
        f"auto_kept={total_kept_auto} "
        f"manual_kept={total_kept_manual} "
        f"layer_mode_updates={total_layer_updates} "
        f"written={len(migration_report.written_files)}"
    )

    for map_entry in migration_report.maps:
        print(f"- {map_entry.path} colliding_layers={map_entry.colliding_layers}")
        for update in map_entry.layer_mode_updates:
            print(
                "  layer_mode_update="
                f"id:{update.layer_id} name:{update.layer_name} "
                f"{update.from_mode}->{update.to_mode}"
            )
        if map_entry.warnings:
            for warning in map_entry.warnings:
                print(f"  warning: {warning}")
        for ts in map_entry.tilesets:
            print(f"  tileset={ts.path} used={ts.used_tile_ids}")

    for ts in migration_report.tilesets:
        print(
            "  tileset_summary="
            f"{ts.path} used={ts.used_tile_ids} existing={ts.existing_colliders} "
            f"added={ts.added_colliders} auto_pruned={ts.auto_colliders_pruned} "
            f"auto_kept={ts.auto_colliders_kept} manual_kept={ts.manual_colliders_kept}"
        )

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
