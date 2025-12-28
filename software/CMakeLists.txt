cmake_minimum_required(VERSION 3.15)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

## Project targets for the software subdirectory

## Sources
set(AC_CORE_SOURCES
    src/core/ClockEngine.cpp
    src/core/MoveFSM.cpp
)

set(AC_PERCEPTION_SOURCES
    src/perception/OpenCVPerceptionEngine.cpp
)

## Public include root and subfolders (headers are organized into subdirs)
set(AC_PUBLIC_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(AC_PUBLIC_INCLUDE_CORE ${AC_PUBLIC_INCLUDE_DIR}/core)
set(AC_PUBLIC_INCLUDE_PERCEPTION ${AC_PUBLIC_INCLUDE_DIR}/perception)
set(AC_PUBLIC_INCLUDE_TYPES ${AC_PUBLIC_INCLUDE_DIR}/types)

## Feature flags
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

## Find dependencies
find_package(OpenCV QUIET COMPONENTS core imgproc highgui)
if(OpenCV_FOUND)
    message(STATUS "Found OpenCV: ${OpenCV_VERSION} (${OpenCV_DIR})")
    set(BUILD_PERCEPTION ON)
else()
    message(WARNING "OpenCV not found: perception module will be disabled. To enable, install OpenCV or set OpenCV_DIR.")
    set(BUILD_PERCEPTION OFF)
endif()

# Find Qt5 QML / Quick 
find_package(Qt5 REQUIRED COMPONENTS
    Core
    Gui
    Qml
    Quick
)
## Libraries
add_library(ac_core STATIC ${AC_CORE_SOURCES})
target_include_directories(ac_core PUBLIC
        ${AC_PUBLIC_INCLUDE_DIR}
        ${AC_PUBLIC_INCLUDE_CORE}
        ${AC_PUBLIC_INCLUDE_TYPES}
)
target_compile_features(ac_core PRIVATE cxx_std_20)

if(BUILD_PERCEPTION)
    add_library(ac_perception STATIC ${AC_PERCEPTION_SOURCES})
    target_include_directories(ac_perception PUBLIC
            ${AC_PUBLIC_INCLUDE_DIR}
            ${AC_PUBLIC_INCLUDE_PERCEPTION}
            ${AC_PUBLIC_INCLUDE_TYPES}
    )
    target_compile_features(ac_perception PRIVATE cxx_std_20)
    target_link_libraries(ac_perception PRIVATE ${OpenCV_LIBS})
else()
    # Provide an interface target so consumers can link unconditionally.
    add_library(ac_perception INTERFACE)
    target_include_directories(ac_perception INTERFACE ${AC_PUBLIC_INCLUDE_DIR})
endif()

## Executable
add_executable(automatic_chess_clock
    src/main.cpp
)
target_include_directories(automatic_chess_clock PRIVATE ${AC_PUBLIC_INCLUDE_DIR})
target_link_libraries(automatic_chess_clock PRIVATE ac_core ac_perception)

# Embed QML files into the binary
add_executable(TimeApp
    src/ui/main.cpp
    src/ui/backend.cpp
    src/ui/backend.h
    src/ui/qml.qrc
)
# Link Qt libraries
target_link_libraries(TimeApp
    Qt5::Core
    Qt5::Gui
    Qt5::Qml
    Qt5::Quick
)

# Raspberry Pi optimizations (optional)
if (CMAKE_SYSTEM_PROCESSOR MATCHES "arm")
    target_compile_options(TimeApp PRIVATE -O2)
endif()

## Install / export (optional)
install(TARGETS ac_core ac_perception automatic_chess_clock
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)



