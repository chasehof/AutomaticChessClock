cmake_minimum_required(VERSION 3.15)

## Project targets for the software subdirectory

## Sources
set(AC_CORE_SOURCES
    src/core/ClockEngine.cpp
    src/core/MoveFSM.cpp
    src/core/MoveProcessor.cpp
    src/logger/Logger.cpp
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
## Try multiple methods to locate OpenCV (system packages may register as
## "OpenCV" or "OpenCV 4" or provide a pkg-config entry named opencv4/opencv).
find_package(OpenCV QUIET COMPONENTS core imgproc highgui)
if(NOT OpenCV_FOUND)
    find_package(OpenCV 4 QUIET COMPONENTS core imgproc highgui)
endif()

if(NOT OpenCV_FOUND)
    # Try pkg-config lookup for opencv4/opencv
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(OPENCV_PKG QUIET opencv4)
        if(NOT OPENCV_PKG_FOUND)
            pkg_check_modules(OPENCV_PKG QUIET opencv)
        endif()

        if(OPENCV_PKG_FOUND)
            message(STATUS "Found OpenCV via pkg-config: ${OPENCV_PKG_VERSION}")
            set(OpenCV_INCLUDE_DIRS ${OPENCV_PKG_INCLUDE_DIRS})
            set(OpenCV_LIBS ${OPENCV_PKG_LIBRARIES})
            set(OpenCV_FOUND TRUE)
        endif()
    endif()
endif()

if(NOT OpenCV_FOUND)
    # Try common Debian/packaged CMake install locations (e.g. /usr/lib/*/cmake/opencv4)
    file(GLOB OPENCV_CMAKE_CANDIDATES
        "/usr/lib/*/cmake/opencv4/OpenCVConfig.cmake"
        "/usr/lib/*/cmake/opencv/OpenCVConfig.cmake"
        "/usr/lib/arm-linux-gnueabihf/cmake/opencv4/OpenCVConfig.cmake"
        "/usr/lib/aarch64-linux-gnu/cmake/opencv4/OpenCVConfig.cmake"
        "/usr/local/lib/*/cmake/opencv4/OpenCVConfig.cmake"
    )

    if(OPENCV_CMAKE_CANDIDATES)
        list(GET OPENCV_CMAKE_CANDIDATES 0 _first_opencv_cmake)
        get_filename_component(_first_opencv_dir ${_first_opencv_cmake} DIRECTORY)
        message(STATUS "Found OpenCVConfig.cmake candidate: ${_first_opencv_cmake}")
        message(STATUS "Setting OpenCV_DIR to: ${_first_opencv_dir}")
        set(OpenCV_DIR ${_first_opencv_dir} CACHE PATH "OpenCV CMake dir (autodetected)" FORCE)
        # Try again with OpenCV_DIR set
        find_package(OpenCV QUIET COMPONENTS core imgproc highgui)
        if(NOT OpenCV_FOUND)
            find_package(OpenCV 4 QUIET COMPONENTS core imgproc highgui)
        endif()
    endif()
endif()

if(OpenCV_FOUND)
    message(STATUS "Found OpenCV: ${OpenCV_VERSION} ${OpenCV_DIR}")
    message(STATUS "OpenCV include dirs: ${OpenCV_INCLUDE_DIRS}")
    message(STATUS "OpenCV libs: ${OpenCV_LIBS}")
    set(BUILD_PERCEPTION ON)
else()
    message(WARNING "OpenCV not found: perception module will be disabled. To enable, install OpenCV (libopencv-dev) or set OpenCV_DIR or ensure pkg-config provides opencv4/opencv.")
    set(BUILD_PERCEPTION OFF)
endif()

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
    # Ensure OpenCV include dirs are propagated to the perception target so
    # headers that include <opencv2/opencv.hpp> can be compiled.
    if(OpenCV_INCLUDE_DIRS)
        # Propagate OpenCV include directories to consumers so headers
        # that pull in <opencv2/opencv.hpp> compile when linking against
        # ac_perception (e.g. automatic_chess_clock).
        target_include_directories(ac_perception PUBLIC ${OpenCV_INCLUDE_DIRS})
    endif()
    if(OpenCV_LIBS)
        target_link_libraries(ac_perception PRIVATE ${OpenCV_LIBS})
    endif()
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
if(BUILD_PERCEPTION)
    target_compile_definitions(automatic_chess_clock PRIVATE BUILD_PERCEPTION=1)
else()
    target_compile_definitions(automatic_chess_clock PRIVATE BUILD_PERCEPTION=0)
endif()
target_link_libraries(automatic_chess_clock PRIVATE ac_core ac_perception)

## Install / export (optional)
install(TARGETS ac_core ac_perception automatic_chess_clock
        RUNTIME DESTINATION bin
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib)



