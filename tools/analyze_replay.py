#!/usr/bin/env python3
import argparse
import csv
import hashlib
import math
from pathlib import Path
from typing import Dict, List, Optional, Tuple


def _parse_float(value: str, default: float = 0.0) -> float:
    try:
        return float(value)
    except (TypeError, ValueError):
        return default


def _parse_int(value: str, default: int = 0) -> int:
    try:
        return int(value)
    except (TypeError, ValueError):
        return default


def load_telemetry(path: Path) -> List[Dict[str, object]]:
    rows: List[Dict[str, object]] = []
    with path.open("r", newline="") as f:
        reader = csv.DictReader(f)
        for idx, raw in enumerate(reader):
            replay_tick = _parse_int(raw.get("replay_tick", str(idx)), idx)
            intent_raw = (raw.get("intent", "") or "").strip()
            vel_x = _parse_float(raw.get("vel_x", "0"))
            inferred_intent = 0
            if vel_x > 1e-6:
                inferred_intent = 1
            elif vel_x < -1e-6:
                inferred_intent = -1

            rows.append(
                {
                    "tick": replay_tick,
                    "time": _parse_float(raw.get("time", "0")),
                    "state": raw.get("state", ""),
                    "x": _parse_float(raw.get("pos_x", "0")),
                    "y": _parse_float(raw.get("pos_y", "0")),
                    "dx_signed": _parse_float(raw.get("dx_signed", "nan"), float("nan")),
                    "dx_abs": _parse_float(raw.get("dx_abs", "nan"), float("nan")),
                    "intent": _parse_int(intent_raw, inferred_intent)
                    if intent_raw
                    else inferred_intent,
                    "ground_shape": (raw.get("ground_shape", "") or "").strip().lower(),
                    "wall_correction_x": _parse_float(raw.get("wall_correction_x", "0")),
                }
            )

    # Backward compatibility for older telemetry files without dx_signed/dx_abs.
    prev_x = None
    for row in rows:
        if math.isnan(float(row["dx_signed"])):
            x = float(row["x"])
            row["dx_signed"] = 0.0 if prev_x is None else (x - prev_x)
        if math.isnan(float(row["dx_abs"])):
            row["dx_abs"] = abs(float(row["dx_signed"]))
        prev_x = float(row["x"])
    return rows


def load_tape_event_count(path: Path) -> int:
    with path.open("r", newline="") as f:
        reader = csv.reader(f)
        count = 0
        for row in reader:
            if not row:
                continue
            first = row[0].strip().lower()
            if first in {"tick", "time"}:
                continue
            count += 1
    return count


def slope_metrics(rows: List[Dict[str, object]]) -> Dict[str, float]:
    polygon_rows = [
        r for r in rows if r["ground_shape"] == "polygon" and int(r["intent"]) > 0
    ]
    sample_count = len(polygon_rows)
    if sample_count == 0:
        return {
            "polygon_samples": 0.0,
            "polygon_net_dx": 0.0,
            "polygon_backward_ratio": 0.0,
            "polygon_wall_correction_abs": 0.0,
        }

    net_dx = sum(float(r["dx_signed"]) for r in polygon_rows)
    backward_steps = sum(1 for r in polygon_rows if float(r["dx_signed"]) < 0.0)
    wall_corr_abs = sum(abs(float(r["wall_correction_x"])) for r in polygon_rows)
    return {
        "polygon_samples": float(sample_count),
        "polygon_net_dx": net_dx,
        "polygon_backward_ratio": backward_steps / sample_count,
        "polygon_wall_correction_abs": wall_corr_abs,
    }


def state_hash(rows: List[Dict[str, object]], max_tick: Optional[int] = None) -> str:
    packed = "|".join(
        f"{int(r['tick'])}:{r['state']}"
        for r in rows
        if max_tick is None or int(r["tick"]) <= max_tick
    )
    return hashlib.sha256(packed.encode("utf-8")).hexdigest()


def compare_runs(
    run_a: List[Dict[str, object]], run_b: List[Dict[str, object]]
) -> Dict[str, float]:
    by_tick_a = {int(r["tick"]): r for r in run_a}
    by_tick_b = {int(r["tick"]): r for r in run_b}
    shared_ticks = sorted(set(by_tick_a.keys()) & set(by_tick_b.keys()))

    max_pos_delta = 0.0
    for tick in shared_ticks:
        a = by_tick_a[tick]
        b = by_tick_b[tick]
        dx = float(a["x"]) - float(b["x"])
        dy = float(a["y"]) - float(b["y"])
        max_pos_delta = max(max_pos_delta, math.hypot(dx, dy))

    max_shared_tick = shared_ticks[-1] if shared_ticks else -1
    shared_final_pos_delta = 0.0
    if max_shared_tick >= 0:
        a = by_tick_a[max_shared_tick]
        b = by_tick_b[max_shared_tick]
        dx = float(a["x"]) - float(b["x"])
        dy = float(a["y"]) - float(b["y"])
        shared_final_pos_delta = math.hypot(dx, dy)

    final_a = run_a[-1] if run_a else {"x": 0.0, "y": 0.0}
    final_b = run_b[-1] if run_b else {"x": 0.0, "y": 0.0}
    final_dx = float(final_a["x"]) - float(final_b["x"])
    final_dy = float(final_a["y"]) - float(final_b["y"])
    full_final_pos_delta = math.hypot(final_dx, final_dy)

    return {
        "shared_ticks": float(len(shared_ticks)),
        "max_shared_tick": float(max_shared_tick),
        "max_pos_delta": max_pos_delta,
        "shared_final_pos_delta": shared_final_pos_delta,
        "full_final_pos_delta": full_final_pos_delta,
        "run_a_rows": float(len(run_a)),
        "run_b_rows": float(len(run_b)),
    }


def print_metrics(label: str, metrics: Dict[str, float]) -> None:
    print(f"{label}:")
    for k, v in metrics.items():
        print(f"  {k}: {v:.6f}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Analyze deterministic replay telemetry and slope traversal quality."
    )
    parser.add_argument("--run-a", required=True, help="Telemetry CSV for run A")
    parser.add_argument("--run-b", help="Telemetry CSV for run B (optional)")
    parser.add_argument("--tape", help="Input tape CSV (optional)")
    parser.add_argument(
        "--epsilon",
        type=float,
        default=1e-4,
        help="Allowed max final position delta for deterministic replay",
    )
    parser.add_argument(
        "--max-backward-ratio",
        type=float,
        default=0.20,
        help="Maximum acceptable backward-step ratio on polygon right-intent samples",
    )
    parser.add_argument(
        "--min-polygon-net-dx",
        type=float,
        default=1.0,
        help="Minimum required net signed displacement over polygon right-intent samples",
    )
    parser.add_argument(
        "--strict-length",
        action="store_true",
        help="Fail deterministic compare when telemetry row counts differ.",
    )
    args = parser.parse_args()

    run_a_path = Path(args.run_a)
    run_b_path = Path(args.run_b) if args.run_b else None
    tape_path = Path(args.tape) if args.tape else None

    run_a = load_telemetry(run_a_path)
    if not run_a:
        print(f"ERROR: no telemetry rows in {run_a_path}")
        return 2

    metrics_a = slope_metrics(run_a)
    print_metrics("Run A slope metrics", metrics_a)
    print(f"Run A state hash (full): {state_hash(run_a)}")

    failures: List[str] = []
    if metrics_a["polygon_samples"] > 0:
        if metrics_a["polygon_net_dx"] < args.min_polygon_net_dx:
            failures.append(
                f"polygon_net_dx {metrics_a['polygon_net_dx']:.6f} < {args.min_polygon_net_dx:.6f}"
            )
        if metrics_a["polygon_backward_ratio"] > args.max_backward_ratio:
            failures.append(
                f"polygon_backward_ratio {metrics_a['polygon_backward_ratio']:.6f} > {args.max_backward_ratio:.6f}"
            )

    if run_b_path:
        run_b = load_telemetry(run_b_path)
        if not run_b:
            print(f"ERROR: no telemetry rows in {run_b_path}")
            return 2

        metrics_b = slope_metrics(run_b)
        print_metrics("Run B slope metrics", metrics_b)
        hash_a_full = state_hash(run_a)
        hash_b_full = state_hash(run_b)
        print(f"Run B state hash (full): {hash_b_full}")

        compare = compare_runs(run_a, run_b)
        print_metrics("Run A/B deterministic diff", compare)
        max_shared_tick = int(compare["max_shared_tick"])
        if max_shared_tick >= 0:
            hash_a_shared = state_hash(run_a, max_shared_tick)
            hash_b_shared = state_hash(run_b, max_shared_tick)
            print(f"Run A shared hash: {hash_a_shared}")
            print(f"Run B shared hash: {hash_b_shared}")
        else:
            hash_a_shared = ""
            hash_b_shared = ""

        if compare["shared_ticks"] <= 0:
            failures.append("no shared ticks between run A and run B")
        if compare["shared_final_pos_delta"] > args.epsilon:
            failures.append(
                f"shared_final_pos_delta {compare['shared_final_pos_delta']:.6f} > {args.epsilon:.6f}"
            )
        if hash_a_shared != hash_b_shared:
            failures.append("state_hash mismatch on shared tick window")
        if args.strict_length and len(run_a) != len(run_b):
            failures.append(
                f"row_count mismatch {len(run_a)} != {len(run_b)} (strict-length enabled)"
            )

    if tape_path:
        tape_events = load_tape_event_count(tape_path)
        print(f"Tape events: {tape_events}")

    if failures:
        print("FAIL")
        for item in failures:
            print(f"  - {item}")
        return 1

    print("PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
