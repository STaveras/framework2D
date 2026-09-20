# framework2D

A cross-platform 2D game engine built with C++17, designed for creating 2D games with multiple rendering backends and a structured state machine architecture.

## Example Game: FantasySideScroller

The `bin/fantasySideScroller` directory contains a side-scroller demo
showcasing the engine capabilities.

### Screenshot

![FantasySideScroller Demo](bin/screenshot.bmp)

## Assets

The `/bin/fantasySideScroller` folder contains free-to-use assets from anokolisa.itch.io and others.

## Features

- **Multi-API Rendering**: DirectX 9, Vulkan, OpenGL, and Metal (macOS/iOS)
- **State Machine Architecture**: Easy game state transitions (hub, mini-games, menus)
- **Tile Map Support**: Level editing with Tiled Map Editor (.tmj, .tmx)
- **Animation System**: Sprite-based animations with frame interpolation
- **Collision System**: AABB-based collision detection and physics operators
- **Event-Driven Input**: Keyboard, mouse, and gamepad input handling
- **JSON Data-Driven**: Configuration via JSON for currencies, upgrades, and content

## Build Dependencies

- **DirectX 9 SDK** (June 2010) - Windows only
- **Vulkan SDK**
- **GLM** (math library)
- **GLFW3** (windowing/input)
- **TinyXML2** (parsing)
- **SIMDJSON** (JSON processing)
- **stb** suite (image loading)
- **FastDelegate** (function objects)
- **Visual Leak Detector** (optional, env vars: VLD_INCLUDE, VLD_LIB)

## Project Structure

- **src/** - Engine source (GameState, Game, Engine2D, etc.)
- **framework/** - Framework project (main engine build)
- **bin/** - Built executables
- **fantasySideScroller/** - Example game demonstrating the engine
- **ext/** - External libraries (stb, delegate, glm, etc.)
- **doc/** - Design documents (Fractured Earth)
- **tools/** - Migration/analysis scripts
- **utl/** - Utility tools
- **CMakeLists.txt** - CMake build configuration

## Building

This project uses CMake. On Windows, Visual Studio projects are available
via the .sln and .vcxproj files. 

On macOS, you can build just by running 'make'.

## Support

If you reuse any of this code, please give a shout out or buy me a coffee for support. <3

## Contact

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

Boar support probes, character ground probes, and collision resolution now use
the framework's shared spatial queries. Static collider bounds are indexed once
and refreshed after edits; ordinary ticks update only dynamic objects. Queries
preserve candidate ordering and leave exact collision and support rules intact.

Latest local Windows x64 Debug/DirectX measurements (September 20, 2026), with
VLD disabled in **both** builds, profiling enabled, VSync/overlays off, and a
five-second idle sample after warm-up:

| Metric | Before spatial queries | After spatial queries |
| --- | ---: | ---: |
| Game update per tick | 15.29 ms | 1.21 ms |
| Collision processing per tick | 8.05 ms | 0.59 ms |
| Average FPS | 54.2 | 258.5 |
| p95 frame time | 24.82 ms | 4.80 ms |
| Collision candidate checks per tick | 2,483 | 13.7 |

The scene contains 5,752 objects. Boar and character support queries formerly
visited all of them; the measured indexed queries returned about 3 and 6
candidates respectively. The final measured run rebuilt the static index zero
times. These are local variable-step idle measurements, not a cycle-identical
replay or a frame-rate guarantee for every scene.

Set `AUTO_PROFILE=1` to print inclusive region times and candidate counts on exit.
See [spatial-query architecture, mutation rules, and tests](doc/spatial_queries.md)
for the API contract and commands. All three headless regression suites pass,
including reference-solver comparisons and real-map boar/support checks.

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

On Windows, normal Debug builds exclude Visual Leak Detector, even when it is
installed. Previously, finding `vld.h` automatically loaded it, and allocation
tracing stayed enabled unless `--debug` was passed. Its stack tracing can dominate
frame time. Debug symbols, assertions, and unoptimized application code remain
enabled in the normal Debug configuration.

For leak investigation, build with `msbuild framework.vcxproj
/p:Configuration="Debug (DX)" /p:Platform=x64 /p:FrameworkEnableVLD=true`, or
configure CMake with `-DFRAMEWORK_ENABLE_VLD=ON`. Set `VLD_INCLUDE` to the VLD
include directory and `VLD_LIB` to its lib directory. Then set
`AUTO_MEMORY_DEBUG=1` when launching to enable tracing across all threads.
Rebuild with the option off when returning to normal debugging.

To benchmark an existing Windows Debug executable from the repository root:

```powershell
$env:AUTO_BENCHMARK_SECONDS = '10'
try {
    & .\bin\framework2D_d.exe --dataPath .\bin\fantasySideScroller
} finally {
    Remove-Item Env:AUTO_BENCHMARK_SECONDS
}
```

The benchmark excludes the first update (which can load the level), then warms
up for one second. Local Windows x64 Debug/DirectX measurements on September 19,
2026, with a five-second idle sample and VSync/overlays off:

| Configuration | Average FPS | Mean frame time | p95 frame time |
| --- | ---: | ---: | ---: |
| VLD included, allocation tracing enabled | 5.0 | 200.85 ms | 211.66 ms |
| Normal Debug, VLD excluded | 35.7 | 28.01 ms | 34.57 ms |

This removes a major diagnostic overhead, but does not make unoptimized Debug
a locked 60 FPS build. Temporary timing instrumentation also identified game
updates as the remaining dominant cost; rendering was roughly 3–4 ms per frame.

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
