cmake_minimum_required(VERSION 3.16)

if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()

# The iOS app build (see FRAMEWORK_IOS below) needs to be iOS before project():
#   cmake -S . -B build/ios -G Xcode -DFRAMEWORK_IOS=ON
if(FRAMEWORK_IOS)
  set(CMAKE_SYSTEM_NAME iOS)
  # Must precede project(), which initializes the languages.
  set(CMAKE_OSX_DEPLOYMENT_TARGET "16.0" CACHE STRING "Minimum iOS version")
endif()

if(MSVC AND NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC runtime library" FORCE)
endif()

if(DEFINED VCPKG_TARGET_TRIPLET AND VCPKG_TARGET_TRIPLET MATCHES ".*-static.*")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC runtime library" FORCE)
elseif(DEFINED VCPKG_TARGET_TRIPLET AND NOT VCPKG_TARGET_TRIPLET STREQUAL "")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC runtime library" FORCE)
endif()

get_filename_component(ROOT_DIR_NAME "${CMAKE_SOURCE_DIR}" NAME)
project(${ROOT_DIR_NAME} LANGUAGES CXX)

# Single-config generators otherwise silently build without optimization.
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release RelWithDebInfo MinSizeRel)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(THREADS_PREFER_PTHREAD_FLAG ON)

if(FRAMEWORK_IOS)
  # ------------------------------------------------------------------
  # iOS app build (Metal renderer, touch controls), configured with:
  #   cmake -S . -B build/ios -G Xcode -DFRAMEWORK_IOS=ON
  # The engine sources below are the same globs as the desktop build,
  # minus the desktop-only files, so there is no separate list to keep
  # in sync with the Makefile.
  # ------------------------------------------------------------------
  if(NOT CMAKE_GENERATOR STREQUAL "Xcode")
    message(FATAL_ERROR "Generate the iOS project with -G Xcode")
  endif()

  enable_language(OBJCXX)
  set(CMAKE_XCODE_EFFECTIVE_PLATFORMS "-iphoneos;-iphonesimulator")
  set(CMAKE_XCODE_GENERATE_SCHEME ON)

  set(SRC_DIR "${CMAKE_SOURCE_DIR}/src")
  set(GAME_DATA_DIR "${CMAKE_SOURCE_DIR}/bin/fantasySideScroller")

  set(FRAMEWORK_IOS_BUNDLE_ID "com.stantaveras.framework2D" CACHE STRING "App bundle identifier")
  set(FRAMEWORK_IOS_TEAM "" CACHE STRING "Development team ID for signing device builds")

  file(STRINGS "${SRC_DIR}/Types.h" _version_line REGEX "#define FRAMEWORK_VERSION ")
  string(REGEX REPLACE ".*\"([^\"]+)\".*" "\\1" FRAMEWORK_VERSION "${_version_line}")

  # simdjson: the pinned single-file amalgamation.
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

  # Everything under src/ except the desktop entry point and the GLFW/
  # OpenGL/Vulkan/Windows platform backends, which src/iOS replaces.
  file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${SRC_DIR}/*.cpp")
  list(FILTER SOURCES EXCLUDE REGEX
    "/(main|RendererGL|RendererVK|TextureGL|TextureVK|Platform[A-Z][^/]*)\\.cpp$")
  file(GLOB_RECURSE APPLE_OBJCXX_SOURCES CONFIGURE_DEPENDS "${SRC_DIR}/*.mm")
  list(APPEND SOURCES ${APPLE_OBJCXX_SOURCES})
  file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    "${SRC_DIR}/*.h"
    "${SRC_DIR}/*.hpp")
  file(GLOB IOS_HEADERS "${SRC_DIR}/iOS/*.h")

  # App icon (the repository's logo.png) and the launch screen colour.
  set(ASSET_CATALOG "${CMAKE_BINARY_DIR}/Assets.xcassets")
  configure_file("${CMAKE_SOURCE_DIR}/logo.png" "${ASSET_CATALOG}/AppIcon.appiconset/logo.png" COPYONLY)
  file(WRITE "${ASSET_CATALOG}/Contents.json"
    "{ \"info\" : { \"author\" : \"xcode\", \"version\" : 1 } }\n")
  file(WRITE "${ASSET_CATALOG}/AppIcon.appiconset/Contents.json"
    "{ \"images\" : [ { \"filename\" : \"logo.png\", \"idiom\" : \"universal\", \"platform\" : \"ios\", \"size\" : \"1024x1024\" } ],\n"
    "  \"info\" : { \"author\" : \"xcode\", \"version\" : 1 } }\n")
  file(WRITE "${ASSET_CATALOG}/LaunchBackground.colorset/Contents.json"
    "{ \"colors\" : [ { \"color\" : { \"color-space\" : \"srgb\", \"components\" : { \"alpha\" : \"1.000\", \"blue\" : \"0.000\", \"green\" : \"0.000\", \"red\" : \"0.000\" } }, \"idiom\" : \"universal\" } ],\n"
    "  \"info\" : { \"author\" : \"xcode\", \"version\" : 1 } }\n")

  add_executable(${ROOT_DIR_NAME} MACOSX_BUNDLE
    ${SOURCES}
    ${HEADERS}
    "${SIMDJSON_DIR}/simdjson.cpp"
    "${ASSET_CATALOG}")

  # The engine's headers pull in Metal, so its C++ is compiled as Objective-C++,
  # as the macOS builds do.
  set_source_files_properties(${SOURCES} PROPERTIES LANGUAGE OBJCXX)

  target_include_directories(${ROOT_DIR_NAME} PRIVATE
    "${CMAKE_SOURCE_DIR}/ext"
    "${CMAKE_SOURCE_DIR}/ext/inc"
    "${SIMDJSON_DIR}"
    "${GLM_INCLUDE_DIR}")

  target_compile_definitions(${ROOT_DIR_NAME} PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<NOT:$<CONFIG:Debug>>:NDEBUG>)

  target_compile_options(${ROOT_DIR_NAME} PRIVATE
    -Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function)

  target_link_libraries(${ROOT_DIR_NAME} PRIVATE
    "-framework CoreGraphics"
    "-framework Foundation"
    "-framework GameController"
    "-framework Metal"
    "-framework QuartzCore"
    "-framework UIKit")

  set_target_properties(${ROOT_DIR_NAME} PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_LIST_DIR}/platform/ios/Info.plist"
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
  add_custom_command(TARGET ${ROOT_DIR_NAME} POST_BUILD
    COMMAND "${CMAKE_CURRENT_LIST_DIR}/platform/ios/copy-game-data.sh" "${GAME_DATA_DIR}"
    COMMENT "Copying game data into the app bundle"
    VERBATIM)

  source_group("simdjson" FILES "${SIMDJSON_DIR}/simdjson.cpp")
  source_group("Platform" FILES ${IOS_HEADERS})
  return()
endif()

find_package(Threads REQUIRED)

if(APPLE)
  enable_language(OBJCXX)

  # Default to the host architecture to avoid accidental universal builds.
  # CMAKE_HOST_SYSTEM_PROCESSOR is the architecture of the *running cmake
  # process*, not the host: an x86_64 cmake (e.g. /usr/local/bin/cmake under
  # Rosetta) would misreport x86_64 on Apple Silicon and break the build
  # against arm64-only Homebrew libraries. Probe sysctl first.
  if(NOT CMAKE_OSX_ARCHITECTURES)
    execute_process(
      COMMAND /usr/sbin/sysctl -n hw.optional.arm64
      OUTPUT_VARIABLE _apple_silicon
      OUTPUT_STRIP_TRAILING_WHITESPACE
      ERROR_VARIABLE _sysctl_error
    )
    if(_apple_silicon EQUAL 1)
      set(_default_arch "arm64")
    elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(x86_64|AMD64)$")
      set(_default_arch "x86_64")
    else()
      execute_process(
        COMMAND uname -m
        OUTPUT_VARIABLE _uname_arch
        OUTPUT_STRIP_TRAILING_WHITESPACE
      )
      if(_uname_arch MATCHES "^(arm64|aarch64)$")
        set(_default_arch "arm64")
      else()
        set(_default_arch "x86_64")
      endif()
    endif()

    set(CMAKE_OSX_ARCHITECTURES "${_default_arch}" CACHE STRING "macOS build architecture" FORCE)
  endif()

  message(STATUS "macOS build architecture: ${CMAKE_OSX_ARCHITECTURES}")
endif()

set(_vcpkg_roots "")
if(DEFINED ENV{VCPKG_ROOT} AND NOT "$ENV{VCPKG_ROOT}" STREQUAL "")
  list(APPEND _vcpkg_roots "$ENV{VCPKG_ROOT}")
endif()
if(WIN32)
  if(DEFINED ENV{USERPROFILE} AND NOT "$ENV{USERPROFILE}" STREQUAL "")
    list(APPEND _vcpkg_roots "$ENV{USERPROFILE}/vcpkg")
  endif()
  list(APPEND _vcpkg_roots
    "C:/vcpkg"
    "${CMAKE_SOURCE_DIR}/vcpkg"
    "${CMAKE_SOURCE_DIR}/../vcpkg"
  )
  if(DEFINED ENV{ProgramFiles} AND NOT "$ENV{ProgramFiles}" STREQUAL "")
    list(APPEND _vcpkg_roots
      "$ENV{ProgramFiles}/Microsoft Visual Studio/2022/Community/VC/vcpkg"
      "$ENV{ProgramFiles}/Microsoft Visual Studio/2022/Professional/VC/vcpkg"
      "$ENV{ProgramFiles}/Microsoft Visual Studio/2022/Enterprise/VC/vcpkg"
      "$ENV{ProgramFiles}/Microsoft Visual Studio/2022/BuildTools/VC/vcpkg"
    )
  endif()
endif()

set(_vcpkg_root "")
if(DEFINED CMAKE_TOOLCHAIN_FILE AND NOT "${CMAKE_TOOLCHAIN_FILE}" STREQUAL "")
  string(REPLACE "\\" "/" _toolchain_norm "${CMAKE_TOOLCHAIN_FILE}")
  if(_toolchain_norm MATCHES "(.+)/scripts/buildsystems/vcpkg\\.cmake$")
    set(_vcpkg_root "${CMAKE_MATCH_1}")
  endif()
endif()
if(NOT _vcpkg_root)
  foreach(_candidate IN LISTS _vcpkg_roots)
    if(EXISTS "${_candidate}/scripts/buildsystems/vcpkg.cmake" OR EXISTS "${_candidate}/installed")
      set(_vcpkg_root "${_candidate}")
      break()
    endif()
  endforeach()
endif()

if(NOT DEFINED VCPKG_TARGET_TRIPLET OR "${VCPKG_TARGET_TRIPLET}" STREQUAL "")
  if(DEFINED ENV{VCPKG_DEFAULT_TRIPLET} AND NOT "$ENV{VCPKG_DEFAULT_TRIPLET}" STREQUAL "")
    set(VCPKG_TARGET_TRIPLET "$ENV{VCPKG_DEFAULT_TRIPLET}" CACHE STRING "vcpkg target triplet")
  elseif(WIN32)
    set(VCPKG_TARGET_TRIPLET "x64-windows" CACHE STRING "vcpkg target triplet")
  endif()
endif()

set(_vcpkg_installed_dirs "")
if(_vcpkg_root AND VCPKG_TARGET_TRIPLET)
  list(APPEND _vcpkg_installed_dirs "${_vcpkg_root}/installed/${VCPKG_TARGET_TRIPLET}")
endif()
if(VCPKG_TARGET_TRIPLET)
  list(APPEND _vcpkg_installed_dirs "${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")
endif()

foreach(_vcpkg_dir IN LISTS _vcpkg_installed_dirs)
  if(EXISTS "${_vcpkg_dir}")
    list(PREPEND CMAKE_PREFIX_PATH "${_vcpkg_dir}")
  endif()
endforeach()

if(WIN32 AND _vcpkg_root)
  message(STATUS "Using inferred vcpkg root: ${_vcpkg_root}")
endif()

find_package(OpenGL REQUIRED)
find_package(glm CONFIG QUIET)
find_package(tinyxml2 CONFIG QUIET)
find_package(TinyXML2 QUIET)
find_package(simdjson CONFIG QUIET)
find_package(glfw3 CONFIG QUIET)
find_package(Vulkan REQUIRED)

if(NOT TARGET glm::glm)
  find_path(GLM_INCLUDE_DIR glm/glm.hpp
    HINTS
      "$ENV{VK_SDK_PATH}/Include"
      "$ENV{VCPKG_ROOT}/installed/x64-windows/include"
      "$ENV{VCPKG_ROOT}/installed/x86-windows/include"
      "${_vcpkg_root}/installed/${VCPKG_TARGET_TRIPLET}/include"
      "${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include"
  )
  if(NOT GLM_INCLUDE_DIR)
    message(FATAL_ERROR "GLM not found. Install glm or set glm_DIR, or ensure VK_SDK_PATH points to a Vulkan SDK that includes glm headers.")
  endif()
endif()

if(
  NOT TARGET tinyxml2::tinyxml2
  AND NOT TARGET unofficial-tinyxml2::unofficial-tinyxml2
  AND NOT TinyXML2_FOUND
)
  find_path(TINYXML2_INCLUDE_DIR tinyxml2.h
    HINTS
      "$ENV{VCPKG_ROOT}/installed/x64-windows/include"
      "$ENV{VCPKG_ROOT}/installed/x86-windows/include"
      "${_vcpkg_root}/installed/${VCPKG_TARGET_TRIPLET}/include"
      "${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include"
  )
  find_library(TINYXML2_LIBRARY NAMES tinyxml2
    HINTS
      "$ENV{VCPKG_ROOT}/installed/x64-windows/lib"
      "$ENV{VCPKG_ROOT}/installed/x86-windows/lib"
      "${_vcpkg_root}/installed/${VCPKG_TARGET_TRIPLET}/lib"
      "${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/lib"
  )
  if(NOT TINYXML2_INCLUDE_DIR OR NOT TINYXML2_LIBRARY)
    message(FATAL_ERROR "tinyxml2 not found. Install tinyxml2, set tinyxml2_DIR/TinyXML2_DIR, or provide include/lib paths discoverable by CMake.")
  endif()
endif()

if(NOT TARGET simdjson::simdjson)
  find_path(SIMDJSON_INCLUDE_DIR simdjson.h
    HINTS
      "$ENV{VCPKG_ROOT}/installed/x64-windows/include"
      "$ENV{VCPKG_ROOT}/installed/x86-windows/include"
      "${_vcpkg_root}/installed/${VCPKG_TARGET_TRIPLET}/include"
      "${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include"
  )
  find_library(SIMDJSON_LIBRARY NAMES simdjson
    HINTS
      "$ENV{VCPKG_ROOT}/installed/x64-windows/lib"
      "$ENV{VCPKG_ROOT}/installed/x86-windows/lib"
      "${_vcpkg_root}/installed/${VCPKG_TARGET_TRIPLET}/lib"
      "${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/lib"
  )
  if(NOT SIMDJSON_INCLUDE_DIR OR NOT SIMDJSON_LIBRARY)
    message(FATAL_ERROR "simdjson not found. Install simdjson, set simdjson_DIR, or provide include/lib paths discoverable by CMake.")
  endif()
endif()

if(
  NOT TARGET glfw
  AND NOT TARGET glfw3::glfw
  AND NOT TARGET glfw3::glfw3
)
  find_path(GLFW3_INCLUDE_DIR GLFW/glfw3.h
    HINTS
      "$ENV{VCPKG_ROOT}/installed/x64-windows/include"
      "$ENV{VCPKG_ROOT}/installed/x86-windows/include"
      "${_vcpkg_root}/installed/${VCPKG_TARGET_TRIPLET}/include"
      "${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/include"
  )
  find_library(GLFW3_LIBRARY NAMES glfw3 glfw
    HINTS
      "$ENV{VCPKG_ROOT}/installed/x64-windows/lib"
      "$ENV{VCPKG_ROOT}/installed/x86-windows/lib"
      "${_vcpkg_root}/installed/${VCPKG_TARGET_TRIPLET}/lib"
      "${CMAKE_SOURCE_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/lib"
  )
  if(NOT GLFW3_INCLUDE_DIR OR NOT GLFW3_LIBRARY)
    message(FATAL_ERROR "glfw3 not found. Install glfw3, set glfw3_DIR, or provide include/lib paths discoverable by CMake.")
  endif()
endif()

file(GLOB_RECURSE HEADER_DIRECTORIES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/ext/inc/*")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")

if(APPLE)
  file(GLOB APPLE_OBJCXX_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.mm")
  list(APPEND SOURCES ${APPLE_OBJCXX_SOURCES})
  # The Metal backend uses ARC; everything else is manually reference counted.
  set_source_files_properties(
    "${CMAKE_SOURCE_DIR}/src/RendererMTL.mm"
    "${CMAKE_SOURCE_DIR}/src/TextureMTL.mm"
    PROPERTIES COMPILE_OPTIONS -fobjc-arc)
endif()

# Headers are listed so IDE projects show them; they are not compiled.
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
  "${CMAKE_SOURCE_DIR}/src/*.h"
  "${CMAKE_SOURCE_DIR}/src/*.hpp")

add_executable(${ROOT_DIR_NAME} ${SOURCES} ${HEADERS})

include(cmake/SourceGroups.cmake)
framework_apply_source_groups("${CMAKE_SOURCE_DIR}/cmake/SourceGroups.txt" ${SOURCES} ${HEADERS})

option(FRAMEWORK_ENABLE_VLD "Include Visual Leak Detector in Windows Debug builds" OFF)
if(WIN32 AND FRAMEWORK_ENABLE_VLD)
  find_path(VLD_INCLUDE_DIR vld.h HINTS "$ENV{VLD_INCLUDE}")
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(_vld_arch Win64)
  else()
    set(_vld_arch Win32)
  endif()
  find_library(VLD_LIBRARY NAMES vld HINTS "$ENV{VLD_LIB}/${_vld_arch}" "$ENV{VLD_LIB}")
  if(NOT VLD_INCLUDE_DIR OR NOT VLD_LIBRARY)
    message(FATAL_ERROR "FRAMEWORK_ENABLE_VLD requires Visual Leak Detector. Set VLD_INCLUDE and VLD_LIB.")
  endif()
  target_include_directories(${ROOT_DIR_NAME} PRIVATE "$<$<CONFIG:Debug>:${VLD_INCLUDE_DIR}>")
  target_compile_definitions(${ROOT_DIR_NAME} PRIVATE $<$<CONFIG:Debug>:FRAMEWORK_ENABLE_VLD>)
  target_link_libraries(${ROOT_DIR_NAME} PRIVATE "$<$<CONFIG:Debug>:${VLD_LIBRARY}>")
endif()

# Ensure Debug configuration defines _DEBUG and Release-like configs define NDEBUG
# Works across generators (including Xcode) and on Windows/MSVC
# Remove any accidental NDEBUG in Debug and add _DEBUG; add NDEBUG for non-Debug
if(MSVC OR WIN32)
  target_compile_definitions(${ROOT_DIR_NAME} PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<NOT:$<CONFIG:Debug>>:NDEBUG>
  )
else()
  # For non-Windows, many compilers implicitly set NDEBUG in Release, but ensure consistency
  target_compile_definitions(${ROOT_DIR_NAME} PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
  )
endif()

if(WIN32)
  target_compile_definitions(${ROOT_DIR_NAME} PRIVATE _CONSOLE)
endif()

# Place the executable in the project's bin/ directory so relative data paths work
set_target_properties(${ROOT_DIR_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/bin"
)

if(MSVC)
  set_target_properties(${ROOT_DIR_NAME} PROPERTIES
    OUTPUT_NAME_DEBUG "${ROOT_DIR_NAME}_d"
  )

  target_compile_options(${ROOT_DIR_NAME} PRIVATE
    /W3
    /sdl
    /permissive-
    /MP
    /openmp
  )

  target_link_options(${ROOT_DIR_NAME} PRIVATE
    $<$<CONFIG:Debug>:/DEBUG:FASTLINK>
    $<$<CONFIG:Release>:/SUBSYSTEM:WINDOWS>
    $<$<NOT:$<CONFIG:Release>>:/SUBSYSTEM:CONSOLE>
  )
endif()

target_include_directories(${ROOT_DIR_NAME} PRIVATE
  "${CMAKE_SOURCE_DIR}/ext/inc"
  "${CMAKE_SOURCE_DIR}/ext/inc/metal-cpp"
)

if(GLM_INCLUDE_DIR)
  target_include_directories(${ROOT_DIR_NAME} PRIVATE "${GLM_INCLUDE_DIR}")
endif()

if(TINYXML2_INCLUDE_DIR)
  target_include_directories(${ROOT_DIR_NAME} PRIVATE "${TINYXML2_INCLUDE_DIR}")
endif()

if(SIMDJSON_INCLUDE_DIR)
  target_include_directories(${ROOT_DIR_NAME} PRIVATE "${SIMDJSON_INCLUDE_DIR}")
endif()

if(GLFW3_INCLUDE_DIR)
  target_include_directories(${ROOT_DIR_NAME} PRIVATE "${GLFW3_INCLUDE_DIR}")
endif()

foreach(HEADER_DIRECTORY ${HEADER_DIRECTORIES})
  if(IS_DIRECTORY ${HEADER_DIRECTORY})
    target_include_directories(${ROOT_DIR_NAME} PRIVATE ${HEADER_DIRECTORY})
  endif()
endforeach()

if(APPLE)
  target_compile_options(${ROOT_DIR_NAME} PRIVATE -stdlib=libc++ -x objective-c++ -DGL_SILENCE_DEPRECATION)
  target_link_options(${ROOT_DIR_NAME} PRIVATE -stdlib=libc++)
endif()

set(PROJECT_LIBS
  Threads::Threads
  OpenGL::GL
)

if(TARGET tinyxml2::tinyxml2)
  list(APPEND PROJECT_LIBS tinyxml2::tinyxml2)
elseif(TARGET unofficial-tinyxml2::unofficial-tinyxml2)
  list(APPEND PROJECT_LIBS unofficial-tinyxml2::unofficial-tinyxml2)
elseif(TinyXML2_FOUND)
  list(APPEND PROJECT_LIBS ${TinyXML2_LIBRARIES})
else()
  list(APPEND PROJECT_LIBS ${TINYXML2_LIBRARY})
endif()

if(TARGET simdjson::simdjson)
  list(APPEND PROJECT_LIBS simdjson::simdjson)
else()
  list(APPEND PROJECT_LIBS ${SIMDJSON_LIBRARY})
endif()

if(TARGET glfw)
  list(APPEND PROJECT_LIBS glfw)
elseif(TARGET glfw3::glfw)
  list(APPEND PROJECT_LIBS glfw3::glfw)
elseif(TARGET glfw3::glfw3)
  list(APPEND PROJECT_LIBS glfw3::glfw3)
else()
  list(APPEND PROJECT_LIBS ${GLFW3_LIBRARY})
endif()

if(TARGET Vulkan::Vulkan)
  list(APPEND PROJECT_LIBS Vulkan::Vulkan)
else()
  list(APPEND PROJECT_LIBS vulkan)
endif()

if(TARGET glm::glm)
  list(APPEND PROJECT_LIBS glm::glm)
endif()

if(WIN32)
  if(DEFINED ENV{DXSDK_DIR} AND EXISTS "$ENV{DXSDK_DIR}/Include")
    target_include_directories(${ROOT_DIR_NAME} PRIVATE "$ENV{DXSDK_DIR}/Include")

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      set(_dxsdk_lib_dir "$ENV{DXSDK_DIR}/Lib/x64")
    else()
      set(_dxsdk_lib_dir "$ENV{DXSDK_DIR}/Lib/x86")
    endif()

    if(EXISTS "${_dxsdk_lib_dir}")
      target_link_directories(${ROOT_DIR_NAME} PRIVATE "${_dxsdk_lib_dir}")
    endif()

    list(APPEND PROJECT_LIBS d3d9 d3dx9 dxguid dinput8)
  else()
    message(WARNING "DXSDK_DIR is not set or invalid. DirectX 9 headers/libs may be missing.")
  endif()
endif()

if(APPLE)
  find_library(METAL_FRAMEWORK Metal REQUIRED)
  find_library(QUARTZCORE_FRAMEWORK QuartzCore REQUIRED)
  find_library(COCOA_FRAMEWORK Cocoa REQUIRED)

  list(APPEND PROJECT_LIBS
    ${METAL_FRAMEWORK}
    ${QUARTZCORE_FRAMEWORK}
    ${COCOA_FRAMEWORK}
  )

  target_link_options(${ROOT_DIR_NAME} PRIVATE
    "-Wl,-rpath,/opt/homebrew/lib"
    "-Wl,-rpath,/usr/local/lib"
  )
endif()

target_link_libraries(${ROOT_DIR_NAME} PRIVATE ${PROJECT_LIBS})

option(FRAMEWORK_BUILD_TESTS "Build headless behavior and spatial-query regression tests" OFF)
if(FRAMEWORK_BUILD_TESTS)
  enable_testing()
  set(_test_core_sources ${SOURCES})
  list(FILTER _test_core_sources EXCLUDE REGEX "/main[.]cpp$")
  add_library(framework_test_core STATIC ${_test_core_sources})
  foreach(_property INCLUDE_DIRECTORIES COMPILE_DEFINITIONS COMPILE_OPTIONS LINK_DIRECTORIES)
    get_target_property(_value ${ROOT_DIR_NAME} ${_property})
    if(_value)
      set_property(TARGET framework_test_core PROPERTY ${_property} "${_value}")
    endif()
  endforeach()
  target_include_directories(framework_test_core PUBLIC "${CMAKE_SOURCE_DIR}/ext/inc")
  get_target_property(_game_includes ${ROOT_DIR_NAME} INCLUDE_DIRECTORIES)
  target_include_directories(framework_test_core PUBLIC ${_game_includes})
  get_target_property(_game_link_dirs ${ROOT_DIR_NAME} LINK_DIRECTORIES)
  if(_game_link_dirs)
    target_link_directories(framework_test_core PUBLIC ${_game_link_dirs})
  endif()
  target_link_libraries(framework_test_core PUBLIC ${PROJECT_LIBS})
  foreach(_test factory_test boar_smoke_test cyclable_test spatial_query_test collision_spatial_test character_one_way_test tile_flip_test level_props_test render_culling_test image_layer_test section_transition_test)
    if(APPLE)
      set_source_files_properties("tools/${_test}.cpp" PROPERTIES LANGUAGE OBJCXX)
    endif()
    add_executable(${_test} "tools/${_test}.cpp")
    target_link_libraries(${_test} PRIVATE framework_test_core)
    target_compile_definitions(${_test} PRIVATE $<$<CONFIG:Debug>:_DEBUG>)
    if(MSVC)
      target_compile_options(${_test} PRIVATE /UNDEBUG)
    else()
      target_compile_options(${_test} PRIVATE -UNDEBUG)
    endif()
    add_test(NAME ${_test} COMMAND ${_test})
    set_tests_properties(${_test} PROPERTIES WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
  endforeach()
endif()

if(WIN32)
  add_custom_command(
    TARGET ${ROOT_DIR_NAME} POST_BUILD
    COMMAND ${CMAKE_SOURCE_DIR}/utl/compile-shaders.bat
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
else()
  add_custom_command(
    TARGET ${ROOT_DIR_NAME} POST_BUILD
    COMMAND ${CMAKE_SOURCE_DIR}/utl/compile-shaders.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
endif()
