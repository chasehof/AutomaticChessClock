# Toolchain file for cross-compiling to 32-bit ARM (armhf)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR arm)

# User should have these cross compilers installed (packaging varies by distro):
# - arm-linux-gnueabihf-gcc
# - arm-linux-gnueabihf-g++

set(CMAKE_C_COMPILER   arm-linux-gnueabihf-gcc)
set(CMAKE_CXX_COMPILER arm-linux-gnueabihf-g++)

# Root path of the target sysroot (optional)
# set(CMAKE_SYSROOT /path/to/arm/sysroot)

# Find behavior: programs from host, libraries/includes from target sysroot
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

# Helpful defaults
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
