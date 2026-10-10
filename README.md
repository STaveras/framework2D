# framework2D

A cross-platform 2D game engine built with C++17, designed for creating 2D games with multiple rendering backends and a structured state machine architecture.

## Features

- **Multi-API Rendering**: DirectX 9, Vulkan, OpenGL, and Metal (macOS/iOS)
- **State Machine Architecture**: Easy game state transitions (hub, mini-games, menus)
- **Tile Map Support**: Level editing with Tiled Map Editor (.tmj, .tmx)
- **Animation System**: Sprite-based animations with frame interpolation
- **Collision System**: AABB-based collision detection and physics operators
- **Event-Driven Input**: Keyboard, mouse, and gamepad input handling
- **JSON Data-Driven**: Configuration via JSON for currencies, upgrades, and content
- **Cross-Platform**: Windows, Linux, macOS, and iOS

## Example Game: FantasySideScroller

The `bin/fantasySideScroller` directory contains a side-scroller demo showcasing the engine capabilities.

### Screenshot

![FantasySideScroller Demo](bin/screenshot.bmp)

The demo features enemies with patrol, chase, and attack behaviors, demonstrating the collision and state machine systems.

## Assets

The `/bin/fantasySideScroller` folder contains free-to-use assets from anokolisa.itch.io and others.

## Build Dependencies

- **GLM** (math library)
- **GLFW 3.3+** (windowing/input and gamepad mappings)
- **TinyXML2** (parsing)
- **SIMDJSON** (JSON processing)
- **stb** suite (image loading)
- **FastDelegate** (function objects)
- **DirectX 9 SDK** (June 2010) - Windows only
- **Vulkan SDK** - Windows/Linux

## Building

This project uses CMake.

### Windows

Visual Studio projects are available via the .sln and .vcxproj files.

### macOS

Build by running `make`:

```bash
make
```

Run the demo:

```bash
./bin/framework2D --vsync
```

### iOS

Configure the root `CMakeLists.txt` with `-DFRAMEWORK_IOS=ON` to generate an Xcode project for iPhone and iPad (iOS 16 or later) that runs FantasySideScroller with the Metal renderer.

```bash
make ios-run
```

builds for the simulator, installs the app, and launches it.

## Controls

| Key | Action |
|-----|--------|
| ← / A | Move left |
| → / D | Move right |
| ↑ / W | Move up (climb) |
| ↓ / S | Move down |
| Space | Jump |
| Z / Ctrl | Attack |
| Shift | Run |
| E | Interact |
| Esc | Pause |

Gamepads use the standard layout: D-pad to move, A to jump, X to attack, Left Bumper to run.

## Support

If you reuse any of this code, please give a shout out or buy me a coffee for support. <3

## Contact

Contact: stan.taveras@gmail.com