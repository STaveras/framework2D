cmake_minimum_required(VERSION 3.10)

# Project Name
project(framework2D CXX)

# Set C++ Standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -pthread")
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Use Objective-C++ on macOS
if(APPLE)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -x objective-c++")
endif()

# Include external libraries
file(GLOB_RECURSE HEADER_DIRECTORIES "${CMAKE_SOURCE_DIR}/ext/inc/*")
include_directories("${CMAKE_SOURCE_DIR}/ext/inc")
foreach(HEADER_DIRECTORY ${HEADER_DIRECTORIES})
  if(IS_DIRECTORY ${HEADER_DIRECTORY})
    include_directories(${HEADER_DIRECTORY})
  endif()
endforeach()

# Platform-specific Configurations
if(WIN32)
  find_path(DirectX_INCLUDE_DIR $ENV{DXSDK_DIR}/Include)
  find_library(DirectX_LIBRARY $ENV{DXSDK_DIR}/Lib)
  include_directories(${DirectX_INCLUDE_DIR})
  link_directories(${DirectX_LIBRARY})
  set(DirectX_LIBRARIES "dxguid.lib;...")
  
  # Vulkan
  find_path(Vulkan_INCLUDE_DIR $ENV{VK_SDK_PATH}/Include)
  find_library(Vulkan_LIBRARY $ENV{VK_SDK_PATH}/Lib)
  include_directories(${Vulkan_INCLUDE_DIR})
  link_directories(${Vulkan_LIBRARY})
elseif(APPLE)
  find_library(METAL_LIBRARY Metal)
  find_library(COCOA_LIBRARY Cocoa)
  find_library(MOLTENVK_LIBRARY MoltenVK)
  set(MACOS_LIBRARIES ${METAL_LIBRARY} ${COCOA_LIBRARY} ${MOLTENVK_LIBRARY})

  # Universal binary (arm64 and x86_64)
  set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
endif()

find_library(GLFW_LIBRARY NAMES glfw3)
find_path(GLFW_INCLUDE_DIR GLFW/glfw3.h)

# Libraries
find_package(GLM REQUIRED)
find_package(TinyXML2 REQUIRED)
find_package(SIMDJSON REQUIRED)
find_package(glfw3 REQUIRED)

# Include Directories
include_directories(${GLM_INCLUDE_DIRS} ${GLFW_INCLUDE_DIR} ${TinyXML2_INCLUDE_DIRS} ${SIMDJSON_INCLUDE_DIRS})

# Source Files
file(GLOB SOURCES "src/*.cpp")

# Executable
add_executable(MyProject ${SOURCES})
target_link_libraries(MyProject ${GLM_LIBRARIES} glfw ${TinyXML2_LIBRARIES} ${SIMDJSON_LIBRARIES} ${CMAKE_THREAD_LIBS_INIT})

if(WIN32)
  target_link_libraries(MyProject ${DirectX_LIBRARIES} ${Vulkan_LIBRARIES})
elseif(APPLE)
  target_link_libraries(MyProject ${MACOS_LIBRARIES})
endif()

# Shaders Compilation
if(WIN32)
  add_custom_command(
    TARGET MyProject POST_BUILD
    COMMAND ${CMAKE_SOURCE_DIR}/utl/compile-shaders.bat
  )
else()
  add_custom_command(
    TARGET MyProject POST_BUILD
    COMMAND ${CMAKE_SOURCE_DIR}/utl/compile-shaders.sh
  )
endif()

# Linux Architecture Detection (Optional)
if(UNIX AND NOT APPLE)
  if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    # 64-bit system
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m64")
  else()
    # 32-bit system
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
  endif()
endif()
