#!/usr/bin/env bash
set -euo pipefail

cd "$(dirname "$0")"
mkdir -p tmp

replay_path="${1:-tmp/auto_input_events.csv}"
telemetry_path="${2:-tmp/auto_slope_telemetry_replay.csv}"
fixed_dt_ms="${3:-${AUTO_FIXED_DT_MS:-16.6667}}"
shift $(( $# > 0 ? 1 : 0 )) || true
shift $(( $# > 0 ? 1 : 0 )) || true
shift $(( $# > 0 ? 1 : 0 )) || true

movement_telemetry="${AUTO_MOVEMENT_TELEMETRY:-${AUTO_SLOPE_TELEMETRY:-1}}"
movement_log_path="${AUTO_MOVEMENT_LOG_PATH:-${AUTO_SLOPE_LOG_PATH:-$telemetry_path}}"

AUTO_DETERMINISTIC=1 \
AUTO_FIXED_DT_MS="$fixed_dt_ms" \
AUTO_INPUT_REPLAY=1 \
AUTO_INPUT_REPLAY_PATH="$replay_path" \
AUTO_MOVEMENT_TELEMETRY="$movement_telemetry" \
AUTO_MOVEMENT_LOG_PATH="$movement_log_path" \
AUTO_SLOPE_TELEMETRY="$movement_telemetry" \
AUTO_SLOPE_LOG_PATH="$movement_log_path" \
./bin/framework2D_d --opengl "$@"
