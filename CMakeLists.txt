cmake_minimum_required(VERSION 3.16)

project(framework2D LANGUAGES CXX)

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

find_package(OpenGL REQUIRED)
find_package(GLM REQUIRED)
find_package(TinyXML2 REQUIRED)
find_package(SIMDJSON REQUIRED)
find_package(glfw3 REQUIRED)
find_package(Vulkan REQUIRED)

file(GLOB_RECURSE HEADER_DIRECTORIES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/ext/inc/*")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.cpp")

if(APPLE)
  file(GLOB APPLE_OBJCXX_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/src/*.mm")
  list(APPEND SOURCES ${APPLE_OBJCXX_SOURCES})
endif()

add_executable(MyProject ${SOURCES})

target_include_directories(MyProject PRIVATE
  "${CMAKE_SOURCE_DIR}/ext/inc"
  "${CMAKE_SOURCE_DIR}/ext/inc/metal-cpp"
  ${GLM_INCLUDE_DIRS}
  ${GLFW_INCLUDE_DIR}
  ${TinyXML2_INCLUDE_DIRS}
  ${SIMDJSON_INCLUDE_DIRS}
)

foreach(HEADER_DIRECTORY ${HEADER_DIRECTORIES})
  if(IS_DIRECTORY ${HEADER_DIRECTORY})
    target_include_directories(MyProject PRIVATE ${HEADER_DIRECTORY})
  endif()
endforeach()

target_compile_options(MyProject PRIVATE -pthread)

if(APPLE)
  target_compile_options(MyProject PRIVATE -stdlib=libc++ -x objective-c++ -DGL_SILENCE_DEPRECATION)
  target_link_options(MyProject PRIVATE -stdlib=libc++)
endif()

set(PROJECT_LIBS
  ${CMAKE_THREAD_LIBS_INIT}
  OpenGL::GL
)

if(TARGET tinyxml2::tinyxml2)
  list(APPEND PROJECT_LIBS tinyxml2::tinyxml2)
else()
  list(APPEND PROJECT_LIBS tinyxml2)
endif()

if(TARGET simdjson::simdjson)
  list(APPEND PROJECT_LIBS simdjson::simdjson)
else()
  list(APPEND PROJECT_LIBS simdjson)
endif()

if(TARGET glfw)
  list(APPEND PROJECT_LIBS glfw)
elseif(TARGET glfw3::glfw)
  list(APPEND PROJECT_LIBS glfw3::glfw)
elseif(TARGET glfw3::glfw3)
  list(APPEND PROJECT_LIBS glfw3::glfw3)
else()
  list(APPEND PROJECT_LIBS glfw)
endif()

if(TARGET Vulkan::Vulkan)
  list(APPEND PROJECT_LIBS Vulkan::Vulkan)
else()
  list(APPEND PROJECT_LIBS vulkan)
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

  target_link_options(MyProject PRIVATE
    "-Wl,-rpath,/opt/homebrew/lib"
    "-Wl,-rpath,/usr/local/lib"
  )
endif()

target_link_libraries(MyProject PRIVATE ${PROJECT_LIBS})

if(WIN32)
  add_custom_command(
    TARGET MyProject POST_BUILD
    COMMAND ${CMAKE_SOURCE_DIR}/utl/compile-shaders.bat
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
else()
  add_custom_command(
    TARGET MyProject POST_BUILD
    COMMAND ${CMAKE_SOURCE_DIR}/utl/compile-shaders.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
endif()
