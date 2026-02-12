cmake_minimum_required(VERSION 3.16)

if(POLICY CMP0091)
  cmake_policy(SET CMP0091 NEW)
endif()

if(DEFINED VCPKG_TARGET_TRIPLET AND VCPKG_TARGET_TRIPLET MATCHES ".*-static.*")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC runtime library" FORCE)
elseif(DEFINED VCPKG_TARGET_TRIPLET AND NOT VCPKG_TARGET_TRIPLET STREQUAL "")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreadedDLL$<$<CONFIG:Debug>:Debug>" CACHE STRING "MSVC runtime library" FORCE)
endif()

get_filename_component(ROOT_DIR_NAME "${CMAKE_SOURCE_DIR}" NAME)
project(${ROOT_DIR_NAME} LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

if(APPLE)
  enable_language(OBJCXX)

  # Default to the host architecture to avoid accidental universal builds.
  if(NOT CMAKE_OSX_ARCHITECTURES)
    if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "^(arm64|aarch64)$")
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
endif()

add_executable(${ROOT_DIR_NAME} ${SOURCES})

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

# Place the executable in the project's bin/ directory so relative data paths work
set_target_properties(${ROOT_DIR_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_SOURCE_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_SOURCE_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_SOURCE_DIR}/bin"
  RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_SOURCE_DIR}/bin"
)

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

