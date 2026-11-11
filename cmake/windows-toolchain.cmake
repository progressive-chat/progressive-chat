# SPDX-License-Identifier: AGPL-3.0-only
# Windows cross-compilation toolchain for Linux → x86_64 Windows.
# Uses clang + mingw-w64 + lld. Run on Linux, produces .exe for Windows.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_VERSION 10.0)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Compiler: clang targeting Windows via mingw-w64
set(CMAKE_C_COMPILER clang-23)
set(CMAKE_CXX_COMPILER clang++-23)
set(CMAKE_RC_COMPILER llvm-rc-23)

# Target triple for Windows GNU (mingw) - use clang with mingw-compatible flags
# -fno-builtin-cpuid avoids conflict between clang's cpuid.h and mingw's intrin.h
set(CMAKE_C_FLAGS_INIT "--target=x86_64-pc-windows-gnu -fno-builtin-cpuid -D__GNUC__=0")
set(CMAKE_CXX_FLAGS_INIT "--target=x86_64-pc-windows-gnu -fno-builtin-cpuid -D__GNUC__=0")
set(CMAKE_RC_FLAGS_INIT "--target=x86_64-pc-windows-gnu")

# Linker: lld (COFF)
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld -Wl,--subsystem,windows")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=lld")

# mingw-w64 sysroot
set(CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32)
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# pkg-config ne nuzhen — Windows biblioteki ne ego ispolzuyut
set(PKG_CONFIG_EXECUTABLE "/usr/bin/false")

# Windows-specific: .exe suffix
set(CMAKE_EXECUTABLE_SUFFIX ".exe")
set(CMAKE_SHARED_LIBRARY_SUFFIX ".dll")
set(CMAKE_STATIC_LIBRARY_SUFFIX ".lib")

# Windows subsystem: GUI (ne konsol)
set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT} -Wl,--subsystem,windows")

# Resource compiler rabotaet
set(CMAKE_RC_COMPILER_WORKS 1)

# Otklyuchaem pkg-config dlya Windows
set(CMAKE_FIND_PACKAGE_NAME "pkg-config" "OFF")