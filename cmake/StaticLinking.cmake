# Helper to prefer static libraries during find_library/find_package
# This file is included by the top-level CMake when AC_PREFER_STATIC is ON.

message(STATUS "AC_PREFER_STATIC=ON: preferring static libraries when searching")

# Prefer .a (static) over shared libs when searching
set(CMAKE_FIND_LIBRARY_SUFFIXES ".a" ".lib" ".so" CACHE STRING "Library suffixes" FORCE)

# Turn off building shared libs by default (projects may still override)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)

if(AC_STATIC_RUNTIME)
  message(STATUS "AC_STATIC_RUNTIME=ON: adding -static link flag (may require a suitable toolchain)")
  # This is aggressive: it attempts to link the C runtime statically. Only enable if your cross toolchain supports it.
  add_link_options(-static)
endif()

# When cross-compiling, prefer libraries from the find root (toolchain) if configured
if(CMAKE_CROSSCOMPILING)
  set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
  set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
endif()
