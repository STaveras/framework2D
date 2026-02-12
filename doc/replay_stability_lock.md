# Replay Stability Lock Workflow

This workflow enforces strict movement stability checks after kinematics refactors.

## Canonical modules

- `src/Kinematics2D.h`, `src/Kinematics2D.cpp`
- `src/Telemetry2D.h`, `src/Telemetry2D.cpp`

## Record tapes

Record one tape per scenario (minimum):

- `tmp/tapes/flat_jump_input_events.csv`
- `tmp/tapes/polygon_slope_input_events.csv`
- `tmp/tapes/wall_push_input_events.csv`
- `tmp/tapes/oneway_drop_input_events.csv`

Example recording command:

```bash
AUTO_DETERMINISTIC=1 AUTO_FIXED_DT_MS=16.6667 \
AUTO_INPUT_RECORD=1 AUTO_INPUT_RECORD_PATH=tmp/tapes/flat_jump_input_events.csv \
AUTO_MOVEMENT_TELEMETRY=1 AUTO_MOVEMENT_LOG_PATH=tmp/flat_jump_record_telemetry.csv \
./bin/framework2D_d --opengl
```

## Run strict gate

```bash
./tools/replay_stability_gate.sh
```

Output artifacts are written to `tmp/replay_gate/`.

## Analyzer usage

Deterministic parity:

```bash
./tools/analyze_replay.py --run-a tmp/run_a.csv --run-b tmp/run_b.csv --tape tmp/tape.csv
```

Nondeterministic quality:

```bash
./tools/analyze_replay.py --run-a tmp/run_nondet.csv --tape tmp/tape.csv
```

Optional segment thresholds:

```bash
./tools/analyze_replay.py --run-a tmp/run.csv --segments tools/segments/polygon_slope_segments.csv
```

Segment CSV schema:

`name,start_tick,end_tick[,min_polygon_net_dx,max_polygon_backward_ratio,min_polygon_samples,max_polygon_wall_correction_abs]`

## Pass criteria

- Deterministic runs:
  - shared tick hash match
  - `shared_final_pos_delta <= epsilon`
- Nondeterministic runs:
  - positive polygon net displacement in slope segments
  - backward-step ratio below threshold
- No stale old module names in source/build refs.
