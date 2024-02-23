#!/bin/bash

# Define compiler and flags
CC=clang++
DEBUG_FLAG="-D_DEBUG"
STD_FLAG="-std=c++17"
STDLIB_FLAG="-stdlib=libc++"
FRAMEWORK_FLAGS="-framework Metal -framework QuartzCore"
DIAGNOSTICS_FLAG="-fdiagnostics-color=always"
INCLUDE_FLAG="-I ./ext/**" # Adjusted to correctly reference from one directory above
DEBUG_INFO_FLAG="-g"
OUTPUT_FLAG="-o ./bin/${PWD##*/}" # Adjusted to output in the correct bin directory
DEPENDENCY_FLAGS="-MMD -MF project.d"
LIBRARY_FLAGS="-lglfw -lvulkan -ltinyxml2 -lsimdjson"

# Compile files, prefixing source files with src/ to reference them correctly
$CC $DEBUG_FLAG $STD_FLAG $STDLIB_FLAG -x objective-c++ $FRAMEWORK_FLAGS $DIAGNOSTICS_FLAG $INCLUDE_FLAG $DEBUG_INFO_FLAG \
src/main.cpp src/Engine2D.cpp src/EventSystem.cpp src/ProgramStack.cpp src/InputManager.cpp src/StateMachine.cpp src/ObjectManager.cpp src/Game.cpp \
src/GameObject.cpp src/GameState.cpp src/Frame.cpp src/Animation.cpp src/AnimationManager.cpp src/AnimationUtils.cpp src/Square.cpp src/Camera.cpp \
src/Timer.cpp src/Window.cpp src/Player.cpp src/IRenderer.cpp src/Renderer.cpp src/RendererVK.cpp src/RendererMTL.mm src/TextureVK.cpp src/IInput.cpp \
src/InputEvent.cpp src/Trigger.cpp src/UpdateBackgroundOperator.cpp src/SDSParser.cpp src/PlatformInput.cpp src/PlatformKeyboard.cpp \
src/ImageLoaders.cpp src/Sprite.cpp src/TileSet.cpp src/Debug.cpp \
$OUTPUT_FLAG $DEPENDENCY_FLAGS $LIBRARY_FLAGS
