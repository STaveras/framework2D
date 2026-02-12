#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")/.."

manifest="tools/replay_gate_manifest.csv"
out_dir="tmp/replay_gate"
fixed_dt_ms="${AUTO_FIXED_DT_MS:-16.6667}"
det_duration="18"
nondet_duration="18"
run_build="1"
renderer_flag="--opengl"

usage() {
  cat <<USAGE
Usage: tools/replay_stability_gate.sh [options]

Options:
  --manifest <path>       Manifest CSV (default: tools/replay_gate_manifest.csv)
  --out-dir <path>        Output directory (default: tmp/replay_gate)
  --fixed-dt-ms <float>   Deterministic fixed dt ms (default: 16.6667)
  --det-seconds <int>     Deterministic replay runtime in seconds (default: 18)
  --nondet-seconds <int>  Nondeterministic replay runtime in seconds (default: 18)
  --skip-build            Skip debug build step
  --renderer <flag>       Renderer CLI flag (default: --opengl)
  -h, --help              Show this help
USAGE
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --manifest)
      manifest="$2"
      shift 2
      ;;
    --out-dir)
      out_dir="$2"
      shift 2
      ;;
    --fixed-dt-ms)
      fixed_dt_ms="$2"
      shift 2
      ;;
    --det-seconds)
      det_duration="$2"
      shift 2
      ;;
    --nondet-seconds)
      nondet_duration="$2"
      shift 2
      ;;
    --skip-build)
      run_build="0"
      shift
      ;;
    --renderer)
      renderer_flag="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage
      exit 2
      ;;
  esac
done

if [[ ! -f "$manifest" ]]; then
  echo "ERROR: manifest not found: $manifest" >&2
  exit 2
fi

mkdir -p "$out_dir"

if [[ "$run_build" == "1" ]]; then
  echo "[gate] Building debug target..."
  make DEBUG=1 -j4
fi

run_capture() {
  local tape="$1"
  local telemetry="$2"
  local log="$3"
  local seconds="$4"
  local deterministic="$5"

  local -a env_vars
  env_vars=(
    "AUTO_INPUT_REPLAY=1"
    "AUTO_INPUT_REPLAY_PATH=$tape"
    "AUTO_MOVEMENT_TELEMETRY=1"
    "AUTO_MOVEMENT_LOG_PATH=$telemetry"
    "AUTO_SLOPE_TELEMETRY=1"
    "AUTO_SLOPE_LOG_PATH=$telemetry"
  )

  if [[ "$deterministic" == "1" ]]; then
    env_vars+=(
      "AUTO_DETERMINISTIC=1"
      "AUTO_FIXED_DT_MS=$fixed_dt_ms"
    )
  fi

  ( env "${env_vars[@]}" ./bin/framework2D_d "$renderer_flag" > "$log" 2>&1 &
    pid=$!
    sleep "$seconds"
    kill -TERM "$pid" >/dev/null 2>&1 || true
    wait "$pid" >/dev/null 2>&1 || true
  )

  if [[ ! -f "$telemetry" ]]; then
    echo "ERROR: telemetry was not produced: $telemetry" >&2
    return 1
  fi

  local rows
  rows=$(wc -l < "$telemetry")
  if [[ "$rows" -le 1 ]]; then
    echo "ERROR: telemetry has no samples: $telemetry" >&2
    return 1
  fi
}

failures=0
scenarios=0

# Expected CSV header:
# scenario,tape_path,segments_path
while IFS=, read -r scenario tape_path segments_path; do
  if [[ -z "${scenario// }" ]]; then
    continue
  fi
  if [[ "$scenario" == "scenario" ]]; then
    continue
  fi

  ((scenarios+=1))
  echo "[gate] Scenario: $scenario"

  tape_path="${tape_path//[$'\r\n']}"
  segments_path="${segments_path//[$'\r\n']}"

  if [[ ! -f "$tape_path" ]]; then
    echo "[gate] FAIL ($scenario): missing tape $tape_path"
    ((failures+=1))
    continue
  fi

  det_a_csv="$out_dir/${scenario}_det_a.csv"
  det_b_csv="$out_dir/${scenario}_det_b.csv"
  nondet_csv="$out_dir/${scenario}_nondet.csv"

  det_a_log="$out_dir/${scenario}_det_a.log"
  det_b_log="$out_dir/${scenario}_det_b.log"
  nondet_log="$out_dir/${scenario}_nondet.log"

  if ! run_capture "$tape_path" "$det_a_csv" "$det_a_log" "$det_duration" "1"; then
    echo "[gate] FAIL ($scenario): deterministic pass A capture failed"
    ((failures+=1))
    continue
  fi

  if ! run_capture "$tape_path" "$det_b_csv" "$det_b_log" "$det_duration" "1"; then
    echo "[gate] FAIL ($scenario): deterministic pass B capture failed"
    ((failures+=1))
    continue
  fi

  analyze_det_cmd=(
    ./tools/analyze_replay.py
    --run-a "$det_a_csv"
    --run-b "$det_b_csv"
    --tape "$tape_path"
  )
  if [[ -n "$segments_path" && -f "$segments_path" ]]; then
    analyze_det_cmd+=(--segments "$segments_path")
  fi

  if ! "${analyze_det_cmd[@]}" > "$out_dir/${scenario}_det_analysis.txt"; then
    echo "[gate] FAIL ($scenario): deterministic analysis failed"
    cat "$out_dir/${scenario}_det_analysis.txt" || true
    ((failures+=1))
    continue
  fi

  if ! run_capture "$tape_path" "$nondet_csv" "$nondet_log" "$nondet_duration" "0"; then
    echo "[gate] FAIL ($scenario): nondeterministic capture failed"
    ((failures+=1))
    continue
  fi

  analyze_nondet_cmd=(
    ./tools/analyze_replay.py
    --run-a "$nondet_csv"
    --tape "$tape_path"
  )
  if [[ -n "$segments_path" && -f "$segments_path" ]]; then
    analyze_nondet_cmd+=(--segments "$segments_path")
  fi

  if ! "${analyze_nondet_cmd[@]}" > "$out_dir/${scenario}_nondet_analysis.txt"; then
    echo "[gate] FAIL ($scenario): nondeterministic analysis failed"
    cat "$out_dir/${scenario}_nondet_analysis.txt" || true
    ((failures+=1))
    continue
  fi

  echo "[gate] PASS ($scenario)"
done < "$manifest"

if [[ "$scenarios" -eq 0 ]]; then
  echo "ERROR: no scenarios found in $manifest" >&2
  exit 2
fi

if [[ "$failures" -ne 0 ]]; then
  echo "[gate] FAIL: $failures / $scenarios scenarios failed"
  exit 1
fi

echo "[gate] PASS: all $scenarios scenarios"
