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
