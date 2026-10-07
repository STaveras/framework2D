# iOS build of framework2D running FantasySideScroller, rendered with Metal.
#
#   cmake -S platform/ios -B build/ios -G Xcode
#   xcodebuild -project build/ios/framework2D.xcodeproj -scheme framework2D \
#              -sdk iphonesimulator -configuration Debug build
#
# or `make ios` / `make ios-run` from the repository root. To run on a device,
# set FRAMEWORK_IOS_TEAM to your development team ID (and FRAMEWORK_IOS_BUNDLE_ID
# to an identifier you can sign), then build for iphoneos or open the project
# in Xcode.
#
# Needs glm's headers (brew install glm). simdjson is downloaded at configure
# time: Homebrew only ships it for macOS.
cmake_minimum_required(VERSION 3.24)

set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_OSX_DEPLOYMENT_TARGET "16.0" CACHE STRING "Minimum iOS version")
set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos;-iphonesimulator")
set(CMAKE_XCODE_GENERATE_SCHEME ON)

project(framework2D LANGUAGES C CXX OBJC OBJCXX)

if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
  message(FATAL_ERROR "Generate the iOS project with -G Xcode")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

get_filename_component(REPO_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
set(SRC_DIR "${REPO_DIR}/src")
set(GAME_DATA_DIR "${REPO_DIR}/bin/fantasySideScroller")

set(FRAMEWORK_IOS_BUNDLE_ID "com.stantaveras.framework2D" CACHE STRING "App bundle identifier")
set(FRAMEWORK_IOS_TEAM "" CACHE STRING "Development team ID for signing device builds")

file(STRINGS "${SRC_DIR}/Types.h" _version_line REGEX "#define FRAMEWORK_VERSION ")
string(REGEX REPLACE ".*\"([^\"]+)\".*" "\\1" FRAMEWORK_VERSION "${_version_line}")

# simdjson: the pinned single-file amalgamation
set(SIMDJSON_VERSION 4.6.11)
set(SIMDJSON_DIR "${CMAKE_BINARY_DIR}/_deps/simdjson-${SIMDJSON_VERSION}")
set(_simdjson_files
  "simdjson.h=a5c467c33c7871262eb4c82775490430758939c1c516fbdaeb662f8255860a60"
  "simdjson.cpp=66c818d1a6b3841febef7a8336ff297f0a52e250a436b598bcdecf010d626243")
foreach(_entry IN LISTS _simdjson_files)
  string(REPLACE "=" ";" _entry "${_entry}")
  list(GET _entry 0 _file)
  list(GET _entry 1 _sha256)
  # Skipped when the file is already there with this hash.
  file(DOWNLOAD
    "https://github.com/simdjson/simdjson/releases/download/v${SIMDJSON_VERSION}/${_file}"
    "${SIMDJSON_DIR}/${_file}"
    EXPECTED_HASH SHA256=${_sha256}
    TLS_VERIFY ON
    STATUS _status)
  list(GET _status 0 _code)
  if(NOT _code EQUAL 0)
    list(GET _status 1 _message)
    message(FATAL_ERROR "Downloading simdjson ${SIMDJSON_VERSION} ${_file} failed: ${_message}")
  endif()
endforeach()

# glm is header-only, so the host's copy works for iOS too.
find_path(GLM_INCLUDE_DIR glm/glm.hpp
  HINTS /opt/homebrew/opt/glm/include /usr/local/opt/glm/include
  NO_CMAKE_FIND_ROOT_PATH)
if(NOT GLM_INCLUDE_DIR)
  message(FATAL_ERROR "glm not found. Install it (brew install glm) or set GLM_INCLUDE_DIR.")
endif()

# Keep in sync with COMMON_SRCS in the Makefile, minus the GLFW, OpenGL and
# Vulkan files and main.cpp, which src/iOS replaces.
set(ENGINE_SOURCES
  Engine2D.cpp RenderInterpolation.cpp EventSystem.cpp ProgramStack.cpp InputManager.cpp StateMachine.cpp
  ObjectManager.cpp CollisionSystem.cpp Game.cpp GameObject.cpp GameState.cpp Frame.cpp Animation.cpp
  AnimationManager.cpp AnimationUtils.cpp Camera.cpp InputMap.cpp Timer.cpp
  Window.cpp Player.cpp IRenderer.cpp Renderer.cpp
  InputTapeRecorder.cpp Font.cpp Cursor.cpp
  Physical.cpp Kinematics2D.cpp Telemetry2D.cpp StrUtils.cpp
  InputEvent.cpp IInput.cpp Trigger.cpp
  UpdateBackgroundOperator.cpp SDSParser.cpp System.cpp
  ImageLoaders.cpp Sprite.cpp TileSet.cpp Debug.cpp
  CollidableGroup.cpp Plane.cpp Square.cpp Circle.cpp Polygon.cpp PolygonDecomposition.cpp
  FantasySideScroller/FantasySideScroller.cpp FantasySideScroller/LevelManager.cpp FantasySideScroller/PlayState.cpp
  FantasySideScroller/PauseState.cpp FantasySideScroller/Character.cpp FantasySideScroller/Boar.cpp
  FantasySideScroller/CharacterMovement.cpp FantasySideScroller/CharacterStateSetup.cpp FantasySideScroller/CharacterUpdate.cpp
  FantasySideScroller/TraversalMechanics.cpp FantasySideScroller/LevelProps.cpp BlinkFlash.cpp
  Actor.cpp PlayerController.cpp)
list(TRANSFORM ENGINE_SOURCES PREPEND "${SRC_DIR}/")

set(APPLE_SOURCES
  "${SRC_DIR}/RendererMTL.mm"
  "${SRC_DIR}/TextureMTL.mm"
  "${SRC_DIR}/iOS/main.mm"
  "${SRC_DIR}/iOS/Window.mm"
  "${SRC_DIR}/iOS/Input.mm"
  "${SRC_DIR}/iOS/TouchControls.mm")

file(GLOB IOS_HEADERS "${SRC_DIR}/iOS/*.h")

# App icon (the repository's icon.png) and the launch screen colour.
set(ASSET_CATALOG "${CMAKE_BINARY_DIR}/Assets.xcassets")
configure_file("${REPO_DIR}/icon.png" "${ASSET_CATALOG}/AppIcon.appiconset/icon.png" COPYONLY)
file(WRITE "${ASSET_CATALOG}/Contents.json"
  "{ \"info\" : { \"author\" : \"xcode\", \"version\" : 1 } }\n")
file(WRITE "${ASSET_CATALOG}/AppIcon.appiconset/Contents.json"
  "{ \"images\" : [ { \"filename\" : \"icon.png\", \"idiom\" : \"universal\", \"platform\" : \"ios\", \"size\" : \"1024x1024\" } ],\n"
  "  \"info\" : { \"author\" : \"xcode\", \"version\" : 1 } }\n")
file(WRITE "${ASSET_CATALOG}/LaunchBackground.colorset/Contents.json"
  "{ \"colors\" : [ { \"color\" : { \"color-space\" : \"srgb\", \"components\" : { \"alpha\" : \"1.000\", \"blue\" : \"0.000\", \"green\" : \"0.000\", \"red\" : \"0.000\" } }, \"idiom\" : \"universal\" } ],\n"
  "  \"info\" : { \"author\" : \"xcode\", \"version\" : 1 } }\n")

add_executable(framework2D MACOSX_BUNDLE
  ${ENGINE_SOURCES}
  ${APPLE_SOURCES}
  ${IOS_HEADERS}
  "${SIMDJSON_DIR}/simdjson.cpp"
  "${ASSET_CATALOG}")

# The engine's headers pull in Metal, so its C++ is compiled as Objective-C++,
# as the macOS builds do.
set_source_files_properties(${ENGINE_SOURCES} PROPERTIES LANGUAGE OBJCXX)

target_include_directories(framework2D PRIVATE
  "${REPO_DIR}/ext"
  "${REPO_DIR}/ext/inc"
  "${SIMDJSON_DIR}"
  "${GLM_INCLUDE_DIR}")

target_compile_definitions(framework2D PRIVATE
  $<$<CONFIG:Debug>:_DEBUG>
  $<$<NOT:$<CONFIG:Debug>>:NDEBUG>)

target_compile_options(framework2D PRIVATE
  -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function)

target_link_libraries(framework2D PRIVATE
  "-framework CoreGraphics"
  "-framework Foundation"
  "-framework GameController"
  "-framework Metal"
  "-framework QuartzCore"
  "-framework UIKit")

set_target_properties(framework2D PROPERTIES
  MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist"
  RESOURCE "${ASSET_CATALOG}"
  XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${FRAMEWORK_IOS_BUNDLE_ID}"
  XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${FRAMEWORK_IOS_TEAM}"
  XCODE_ATTRIBUTE_CODE_SIGN_STYLE Automatic
  XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
  XCODE_ATTRIBUTE_IPHONEOS_DEPLOYMENT_TARGET "${CMAKE_OSX_DEPLOYMENT_TARGET}"
  XCODE_ATTRIBUTE_SUPPORTED_PLATFORMS "iphoneos iphonesimulator"
  XCODE_ATTRIBUTE_SUPPORTS_MACCATALYST NO
  XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC YES
  XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
  # The data copy below reads from the repository.
  XCODE_ATTRIBUTE_ENABLE_USER_SCRIPT_SANDBOXING NO)

# The game's data, copied into the bundle on every build.
add_custom_command(TARGET framework2D POST_BUILD
  COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/copy-game-data.sh" "${GAME_DATA_DIR}"
  COMMENT "Copying game data into the app bundle"
  VERBATIM)

source_group("simdjson" FILES "${SIMDJSON_DIR}/simdjson.cpp")
source_group("Platform" FILES ${IOS_HEADERS} ${APPLE_SOURCES})
