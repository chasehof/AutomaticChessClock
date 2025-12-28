#!/usr/bin/env bash
set -euo pipefail

# Simple build helper for AutomaticChessClock
# Supports using CMake presets or manual cross-config via toolchain file.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$SCRIPT_DIR"

JOBS=$(nproc 2>/dev/null || echo 1)
PRESET=""
TOOLCHAIN=""
BUILD_TYPE="RelWithDebInfo"
AC_STATIC_RUNTIME=OFF
AC_PREFER_STATIC=ON
CLEAN=0
CONFIGURE_ONLY=0
BUILD_ONLY=0

usage() {
  cat <<EOF
Usage: $(basename "$0") [options]

Options:
  --preset NAME         Use a configure preset from CMakePresets.json (e.g. host-debug, armhf-release)
  --arch ARCH           Shortcut for presets: host-debug, host-release, armhf, aarch64
  --toolchain PATH      Use a specific CMake toolchain file (overrides preset)
  --build-type TYPE     CMake build type (Debug, Release, RelWithDebInfo). Default: ${BUILD_TYPE}
  --static-runtime      Enable AC_STATIC_RUNTIME (attempt static CRT linking)
  --no-static           Disable AC_PREFER_STATIC
  --jobs N              Number of parallel build jobs. Default: ${JOBS}
  --clean               Remove build directory before configuring
  --configure-only      Configure but don't build
  --build-only          Build using existing configure
  -h, --help            Show this help and exit

Examples:
  $(basename "$0") --preset host-debug
  $(basename "$0") --arch armhf --toolchain cmake/toolchains/armhf.cmake

EOF
}

if [[ $# -eq 0 ]]; then
  usage
  exit 0
fi

while [[ $# -gt 0 ]]; do
  case "$1" in
    --preset)
      PRESET="$2"; shift 2;;
    --preset=*)
      PRESET="${1#*=}"; shift;;
    --arch)
      case "$2" in
        host-debug) PRESET=host-debug;;
        host-release) PRESET=host-release;;
        armhf) PRESET=armhf-release;;
        aarch64) PRESET=aarch64-release;;
        *) echo "Unknown arch preset: $2"; exit 1;;
      esac; shift 2;;
    --toolchain)
      TOOLCHAIN="$2"; shift 2;;
    --toolchain=*)
      TOOLCHAIN="${1#*=}"; shift;;
    --build-type)
      BUILD_TYPE="$2"; shift 2;;
    --build-type=*)
      BUILD_TYPE="${1#*=}"; shift;;
    --static-runtime)
      AC_STATIC_RUNTIME=ON; shift;;
    --no-static)
      AC_PREFER_STATIC=OFF; shift;;
    --jobs)
      JOBS="$2"; shift 2;;
    --jobs=*)
      JOBS="${1#*=}"; shift;;
    --clean)
      CLEAN=1; shift;;
    --configure-only)
      CONFIGURE_ONLY=1; shift;;
    --build-only)
      BUILD_ONLY=1; shift;;
    -h|--help)
      usage; exit 0;;
    *)
      echo "Unknown arg: $1"; usage; exit 1;;
  esac
done

# determine binary dir
if [[ -n "$PRESET" ]]; then
  # map preset to binary dir same as in CMakePresets.json
  case "$PRESET" in
    host-debug) BUILD_DIR="$PROJECT_ROOT/build/host-debug";;
    host-release) BUILD_DIR="$PROJECT_ROOT/build/host-release";;
    armhf-release) BUILD_DIR="$PROJECT_ROOT/build/armhf";;
    aarch64-release) BUILD_DIR="$PROJECT_ROOT/build/aarch64";;
    *) BUILD_DIR="$PROJECT_ROOT/build/${PRESET}";;
  esac
else
  BUILD_DIR="$PROJECT_ROOT/build/${BUILD_TYPE}"
fi

if [[ $CLEAN -eq 1 ]]; then
  echo "Removing build dir: $BUILD_DIR"
  rm -rf "$BUILD_DIR"
fi

if [[ $BUILD_ONLY -eq 1 ]]; then
  if [[ ! -d "$BUILD_DIR" ]]; then
    echo "Build dir does not exist: $BUILD_DIR"; exit 1
  fi
  echo "Building in $BUILD_DIR (-j $JOBS)"
  cmake --build "$BUILD_DIR" -- -j"$JOBS"
  exit 0
fi

if [[ -n "$PRESET" && -z "$TOOLCHAIN" ]]; then
  echo "Configuring using preset: $PRESET"
  cmake --preset "$PRESET"
else
  echo "Manual configure: build dir=$BUILD_DIR"
  mkdir -p "$BUILD_DIR"
  CFG=( -S "$PROJECT_ROOT" -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DAC_PREFER_STATIC="$AC_PREFER_STATIC" -DAC_STATIC_RUNTIME="$AC_STATIC_RUNTIME" )
  if [[ -n "$TOOLCHAIN" ]]; then
    CFG+=( -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" )
  fi
  echo "Running: cmake ${CFG[*]}"
  cmake "${CFG[@]}"
fi

if [[ $CONFIGURE_ONLY -eq 1 ]]; then
  echo "Configure only; skipping build"
  exit 0
fi

echo "Building in $BUILD_DIR (-j $JOBS)"
cmake --build "$BUILD_DIR" -- -j"$JOBS"

echo "Build finished. Binary (if built): $BUILD_DIR/automatic_chess_clock"
