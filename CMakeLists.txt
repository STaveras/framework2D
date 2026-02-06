cmake_minimum_required(VERSION 3.16)

project(framework2D LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if(APPLE)
  enable_language(OBJCXX)
  set(CMAKE_OBJCXX_STANDARD 17)
  set(CMAKE_OBJCXX_STANDARD_REQUIRED ON)
endif()

option(SUPPRESS_UNUSED "Suppress unused warnings" ON)

set(PROJECT_ROOT ${CMAKE_SOURCE_DIR})

set(COMMON_SOURCES
  src/main.cpp
  src/Engine2D.cpp
  src/EventSystem.cpp
  src/ProgramStack.cpp
  src/InputManager.cpp
  src/StateMachine.cpp
  src/ObjectManager.cpp
  src/Game.cpp
  src/GameObject.cpp
  src/GameState.cpp
  src/Frame.cpp
  src/Animation.cpp
  src/AnimationManager.cpp
  src/AnimationUtils.cpp
  src/Square.cpp
  src/Camera.cpp
  src/Controller.cpp
  src/Timer.cpp
  src/Window.cpp
  src/Player.cpp
  src/IRenderer.cpp
  src/Renderer.cpp
  src/RendererVK.cpp
  src/RendererGL.cpp
  src/TextureVK.cpp
  src/TextureGL.cpp
  src/InputEvent.cpp
  src/IInput.cpp
  src/Trigger.cpp
  src/UpdateBackgroundOperator.cpp
  src/SDSParser.cpp
  src/PlatformInput.cpp
  src/PlatformKeyboard.cpp
  src/System.cpp
  src/ImageLoaders.cpp
  src/Sprite.cpp
  src/TileSet.cpp
  src/Debug.cpp
  src/FantasySideScroller/FantasySideScroller.cpp
  src/FantasySideScroller/PlayState.cpp
  src/FantasySideScroller/Character.cpp
)

set(PLATFORM_SOURCES "")
if(APPLE)
  list(APPEND PLATFORM_SOURCES
    src/RendererMTL.mm
    src/TextureMTL.mm
  )
elseif(WIN32)
  list(APPEND PLATFORM_SOURCES
    src/RendererDX.cpp
  )
endif()

add_executable(framework2D ${COMMON_SOURCES} ${PLATFORM_SOURCES})

target_include_directories(framework2D
  PRIVATE
    ${PROJECT_ROOT}/ext
  SYSTEM PRIVATE
    ${PROJECT_ROOT}/ext/inc
    ${PROJECT_ROOT}/ext/inc/metal-cpp
)

find_package(Threads REQUIRED)
target_link_libraries(framework2D PRIVATE Threads::Threads)

find_package(glm QUIET)
if(TARGET glm::glm)
  target_link_libraries(framework2D PRIVATE glm::glm)
elseif(DEFINED GLM_INCLUDE_DIRS)
  target_include_directories(framework2D PRIVATE ${GLM_INCLUDE_DIRS})
endif()

target_compile_options(framework2D PRIVATE
  $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wall -Wextra -fdiagnostics-color=always>
  $<$<CXX_COMPILER_ID:MSVC>:/W3>
)

if(SUPPRESS_UNUSED)
  target_compile_options(framework2D PRIVATE
    $<$<CXX_COMPILER_ID:AppleClang,Clang,GNU>:-Wno-unused-parameter -Wno-unused-variable -Wno-unused-but-set-variable -Wno-unused-function>
  )
endif()

target_compile_definitions(framework2D PRIVATE
  $<$<CONFIG:Debug>:_DEBUG>
)

set_target_properties(framework2D PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${PROJECT_ROOT}/bin"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_ROOT}/bin"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_ROOT}/bin"
  OUTPUT_NAME "framework2D"
  OUTPUT_NAME_DEBUG "framework2D_d"
)

if(APPLE)
  target_compile_definitions(framework2D PRIVATE GL_SILENCE_DEPRECATION)
  target_compile_options(framework2D PRIVATE -stdlib=libc++ -x objective-c++)
  target_link_options(framework2D PRIVATE -stdlib=libc++)

  find_library(METAL_FRAMEWORK Metal)
  find_library(QUARTZCORE_FRAMEWORK QuartzCore)
  find_library(OPENGL_FRAMEWORK OpenGL)
  find_library(COCOA_FRAMEWORK Cocoa)

  target_link_libraries(framework2D PRIVATE
    ${METAL_FRAMEWORK}
    ${QUARTZCORE_FRAMEWORK}
    ${OPENGL_FRAMEWORK}
    ${COCOA_FRAMEWORK}
  )

  find_library(VULKAN_LIBRARY vulkan)
  if(VULKAN_LIBRARY)
    target_link_libraries(framework2D PRIVATE ${VULKAN_LIBRARY})
  else()
    target_link_libraries(framework2D PRIVATE vulkan)
  endif()

  set_target_properties(framework2D PROPERTIES
    BUILD_RPATH "/usr/local/lib;/opt/homebrew/lib"
  )

elseif(WIN32)
  if(DEFINED ENV{DXSDK_DIR})
    target_include_directories(framework2D PRIVATE "$ENV{DXSDK_DIR}/Include")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      target_link_directories(framework2D PRIVATE "$ENV{DXSDK_DIR}/Lib/x64")
    else()
      target_link_directories(framework2D PRIVATE "$ENV{DXSDK_DIR}/Lib/x86")
    endif()
  endif()

  if(DEFINED ENV{VK_SDK_PATH})
    target_include_directories(framework2D PRIVATE "$ENV{VK_SDK_PATH}/Include")
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
      target_link_directories(framework2D PRIVATE "$ENV{VK_SDK_PATH}/Lib")
    else()
      target_link_directories(framework2D PRIVATE "$ENV{VK_SDK_PATH}/Lib32")
    endif()
  endif()

  target_link_libraries(framework2D PRIVATE opengl32)

  find_library(VULKAN_LIBRARY vulkan-1)
  if(VULKAN_LIBRARY)
    target_link_libraries(framework2D PRIVATE ${VULKAN_LIBRARY})
  else()
    target_link_libraries(framework2D PRIVATE vulkan-1)
  endif()
endif()

find_package(glfw3 QUIET)
if(TARGET glfw)
  target_link_libraries(framework2D PRIVATE glfw)
elseif(TARGET glfw3)
  target_link_libraries(framework2D PRIVATE glfw3)
elseif(TARGET glfw3::glfw)
  target_link_libraries(framework2D PRIVATE glfw3::glfw)
else()
  find_library(GLFW_LIB NAMES glfw3 glfw)
  if(GLFW_LIB)
    target_link_libraries(framework2D PRIVATE ${GLFW_LIB})
  else()
    target_link_libraries(framework2D PRIVATE glfw)
  endif()
endif()

find_package(TinyXML2 QUIET)
if(TARGET TinyXML2::TinyXML2)
  target_link_libraries(framework2D PRIVATE TinyXML2::TinyXML2)
elseif(TARGET tinyxml2::tinyxml2)
  target_link_libraries(framework2D PRIVATE tinyxml2::tinyxml2)
else()
  target_link_libraries(framework2D PRIVATE tinyxml2)
endif()

find_package(simdjson QUIET)
if(TARGET simdjson::simdjson)
  target_link_libraries(framework2D PRIVATE simdjson::simdjson)
else()
  target_link_libraries(framework2D PRIVATE simdjson)
endif()

if(WIN32)
  add_custom_command(
    TARGET framework2D POST_BUILD
    COMMAND ${PROJECT_ROOT}/utl/compile-shaders.bat
    WORKING_DIRECTORY ${PROJECT_ROOT}
  )
else()
  add_custom_command(
    TARGET framework2D POST_BUILD
    COMMAND ${PROJECT_ROOT}/utl/compile-shaders.sh
    WORKING_DIRECTORY ${PROJECT_ROOT}
  )
endif()
