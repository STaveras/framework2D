#!/bin/bash

# Initialize optional flags
DEBUG_FLAG=""
DEBUG_INFO_FLAG=""

# Check for --debug argument
for arg in "$@"
do
    if [ "$arg" == "--debug" ]; then
        DEBUG_FLAG="-D_DEBUG"
        DEBUG_INFO_FLAG="-g"
        break
    fi
done

# Define compiler and flags
CC=clang++
STD_FLAG="-std=c++17"
STDLIB_FLAG="-stdlib=libc++"
FRAMEWORK_FLAGS="-framework Metal -framework QuartzCore -framework OpenGL -framework Cocoa"
DIAGNOSTICS_FLAG="-fdiagnostics-color=always"
INCLUDE_FLAG="-I ./ext/**" # Adjusted to correctly reference from one directory above
ADDITIONAL_INC="-I ./ext/inc/metal-cpp" # Adjusted to correctly reference from one directory above
OUTPUT_FLAG="-o ./bin/${PWD##*/}" # Adjusted to output in the correct bin directory
DEPENDENCY_FLAGS="-MMD -MF project.d"
LIBRARY_FLAGS="-lglfw -lvulkan -ltinyxml2 -lsimdjson"

# Compile files, prefixing source files with src/ to reference them correctly
$CC $DEBUG_FLAG $STD_FLAG $STDLIB_FLAG -x objective-c++ $FRAMEWORK_FLAGS $DIAGNOSTICS_FLAG $INCLUDE_FLAG $ADDITIONAL_INC $DEBUG_INFO_FLAG \
src/main.cpp src/Engine2D.cpp src/EventSystem.cpp src/ProgramStack.cpp src/InputManager.cpp src/StateMachine.cpp src/ObjectManager.cpp src/Game.cpp \
src/GameObject.cpp src/GameState.cpp src/Frame.cpp src/Animation.cpp src/AnimationManager.cpp src/AnimationUtils.cpp src/Square.cpp src/Camera.cpp src/Controller.cpp \
src/Timer.cpp src/Window.cpp src/Player.cpp src/IRenderer.cpp src/Renderer.cpp src/RendererVK.cpp src/RendererMTL.mm src/RendererGL.cpp src/TextureVK.cpp src/TextureGL.cpp src/TextureMTL.mm \
src/InputEvent.cpp src/IInput.cpp src/Trigger.cpp src/UpdateBackgroundOperator.cpp src/SDSParser.cpp src/PlatformInput.cpp src/PlatformKeyboard.cpp src/System.cpp \
src/ImageLoaders.cpp src/Sprite.cpp src/TileSet.cpp src/Debug.cpp \
src/FantasySideScroller/FantasySideScroller.cpp \
src/FantasySideScroller/PlayState.cpp \
src/FantasySideScroller/Character.cpp \
$OUTPUT_FLAG $DEPENDENCY_FLAGS $LIBRARY_FLAGS
