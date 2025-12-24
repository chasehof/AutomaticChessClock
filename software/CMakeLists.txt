cmake_minimum_required(VERSION 3.15)
project(AutomaticChessClock
    VERSION 1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(AutomaticChessClock
    src/main.cpp
    src/core/ClockEngine.cpp
    src/core/MoveFSM.cpp
    src/perception/ChessDetector.cpp
)

target_include_directories(AutomaticChessClock PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)



