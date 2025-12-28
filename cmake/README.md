Cross-compiling and static linking helper files for AutomaticChessClock

How to cross-compile

1. Create an out-of-source build dir, choose a toolchain file and run CMake:

   mkdir build-arm && cd build-arm
   cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/armhf.cmake ..
   cmake --build . -- -j

2. For 64-bit aarch64:

   mkdir build-aarch64 && cd build-aarch64
   cmake -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchains/aarch64.cmake ..
   cmake --build . -- -j

Static linking notes

- The project contains an option `AC_PREFER_STATIC` (default ON) that asks CMake to prefer static libraries when searching. This only works if static builds of dependencies (e.g., OpenCV) are available in your sysroot or toolchain.
- If you want to attempt linking the C runtime statically, set `-DAC_STATIC_RUNTIME=ON` when configuring. This will add `-static` to link options and will often require a matching static sysroot.
- Building OpenCV statically for your target is outside the scope of this repository. Typically you must cross-build OpenCV for your target and point CMake at the resulting libraries using `-DOpenCV_DIR=` or by installing them into your toolchain sysroot.

Troubleshooting

- If CMake can't find static OpenCV libraries, either build OpenCV for your target or disable `AC_PREFER_STATIC`.
- For cross compilation, ensure your sysroot contains headers and libraries for the target and that the cross compilers named in the toolchain files are installed or update the toolchain files accordingly.
