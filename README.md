# framework2D
An old code base for making 2D games.

Currently a pet project to reacclimate myself with game development. 

## Build dependencies:
- DirectX 9 SDK (June 2010)
- Vulkan SDK
- GLM
- GLFW3
- TinyXML2
- SIMDJSON
- stb (git submodule)
- FastDelegate (included)
- Visual Leak Detector (environment variables must be set: VLD_INCLUDE, VLD_LIB)

The /bin folder contains free to use assets from anokolisa.itch.io.

If you reuse any of this code, please give a shout out or buy me a coffee for support. <3

Contact: stan.taveras@gmail.com

## Boar enemy

A boar spawns to the right of the entrance in Mosswood Hollow. It patrols,
turns at ledges, and charges nearby players. Contact costs 15 health with a
one-second cooldown. Face it and press Left Ctrl to defeat it; press R to
reset both the player and boar. Idle, walk, run, and hit/vanish animations use
the original `Mob/Boar` sprite sheets.

Run `make test-boar` for headless asset, movement, combat, respawn, and map
collision checks. Build the game with `make`.

## Performance

`make` builds with `-O2`; `make DEBUG=1` keeps an unoptimized debug build.
Release and debug objects are stored separately. Use the release executable
for frame-rate measurements.

Run `tools/benchmark.sh --opengl` for a ten-second benchmark after a one-second
warm-up. It exits automatically and reports average FPS, mean/p95/p99 frame
times, and frames exceeding the 16.67 ms budget for 60 FPS. Set
`AUTO_BENCHMARK_SECONDS` to change the measurement duration. Add `--vsync` to
measure presentation pacing; leave it off to measure rendering throughput.
Existing input replays can be used through `AUTO_INPUT_REPLAY_PATH` for a
repeatable moving-camera workload. Keep window size, replay, and debug-overlay
settings identical when comparing runs.

The OpenGL renderer culls sprites outside the camera bounds and batches
consecutive sprites sharing a texture, preserving transparency/layer order.

Local macOS/OpenGL measurements (640×480 logical window, September 15, 2026):

| Workload | Average FPS | p95 frame time | p99 frame time |
| --- | ---: | ---: | ---: |
| Previous build, idle | 30.7 | 49.49 ms | 62.69 ms |
| Optimized build, idle | 134.9 | 15.37 ms | 16.87 ms |
| Optimized, deterministic flat-jump replay | 144.7 | 18.27 ms | 28.36 ms |
| Optimized, VSync enabled | 102.3 | 16.85 ms | 20.25 ms |

These runs exceed 60 FPS on average; occasional frames still exceed 16.67 ms,
particularly during replay. This is not a guarantee of a locked 60 FPS on every
machine. VSync follows the display's refresh rate, which need not be 60 Hz.
The boar/terrain behavior checks passed with assertions enabled. A framebuffer
comparison against the previous renderer covered 12 combinations of camera
anchor, rotation, and zoom with overlapping translucent, flipped sprites:
0–4 differing color channels out of 1,228,800 per case (tolerance: 2/255),
with no OpenGL errors.
