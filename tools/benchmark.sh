#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
# Pass renderer, vsync, debug-overlay, or other game options through unchanged.
make -j"${BUILD_JOBS:-8}"
AUTO_BENCHMARK_SECONDS="${AUTO_BENCHMARK_SECONDS:-10}" ./bin/"$(basename "$PWD")" "$@"
