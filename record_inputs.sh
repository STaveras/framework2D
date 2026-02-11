#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"
mkdir -p tmp

record_path="${1:-tmp/auto_input_events.csv}"
telemetry_path="${2:-tmp/auto_slope_telemetry.csv}"
fixed_dt_ms="${3:-${AUTO_FIXED_DT_MS:-16.6667}}"
shift $(( $# > 0 ? 1 : 0 )) || true
shift $(( $# > 0 ? 1 : 0 )) || true
shift $(( $# > 0 ? 1 : 0 )) || true

AUTO_DETERMINISTIC=1 \
AUTO_FIXED_DT_MS="$fixed_dt_ms" \
AUTO_INPUT_RECORD=1 \
AUTO_INPUT_RECORD_PATH="$record_path" \
AUTO_SLOPE_TELEMETRY=1 \
AUTO_SLOPE_LOG_PATH="$telemetry_path" \
./bin/framework2D_d --opengl "$@"
