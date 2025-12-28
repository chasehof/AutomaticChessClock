# Toolchain file for cross-compiling to 64-bit ARM (aarch64)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# Expected cross compilers:
# - aarch64-linux-gnu-gcc
# - aarch64-linux-gnu-g++

set(CMAKE_C_COMPILER   aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

# Optional sysroot specification
# set(CMAKE_SYSROOT /path/to/aarch64/sysroot)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
