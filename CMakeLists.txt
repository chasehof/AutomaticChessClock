cmake_minimum_required(VERSION 3.20)
project(AutomaticChessClock
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Prefer out-of-source builds
if("${CMAKE_SOURCE_DIR}" STREQUAL "${CMAKE_BINARY_DIR}")
  message(FATAL_ERROR "In-source builds are not supported. Please create a build directory and run CMake from there.")
endif()

# Default build type
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
endif()

option(AC_PREFER_STATIC "Prefer static libraries when available" ON)
option(AC_STATIC_RUNTIME "Attempt to link the C/C++ runtime statically (may require a static toolchain)" OFF)

# Expose path to our cmake helpers
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

if(AC_PREFER_STATIC)
  include(StaticLinking)
endif()

# Export compile commands for editor integration
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

## Subdirectories
add_subdirectory(software)

## Basic packaging/export support (optional)
include(CMakePackageConfigHelpers)
