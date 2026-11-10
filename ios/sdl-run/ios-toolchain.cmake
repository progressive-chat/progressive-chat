# SPDX-License-Identifier: AGPL-3.0-only

# iOS simulator toolchain for CMake on Linux. Given the SDK path and the
# compiler, it pretends to be an iOS build machine. Linux clang needs
# --target instead of -arch/-mios-simulator-version-min.

set(CMAKE_SYSTEM_NAME iOS)
set(CMAKE_SYSTEM_VERSION 26.0)
set(CMAKE_OSX_SYSROOT ${IOS_SDK_PATH})
set(CMAKE_OSX_ARCHITECTURES arm64)
set(CMAKE_OSX_DEPLOYMENT_TARGET 26.0)

# The compiler is clang 23 from apt.llvm.org.
set(CMAKE_C_COMPILER /usr/bin/clang-23)
set(CMAKE_CXX_COMPILER /usr/bin/clang++-23)
set(CMAKE_OBJC_COMPILER /usr/bin/clang-23)
set(CMAKE_OBJCXX_COMPILER /usr/bin/clang++-23)

# Use lld for Mach-O linking.
set(CMAKE_EXE_LINKER_FLAGS_INIT "-fuse-ld=lld -Wl,-platform_version,ios-simulator,26.0,26.0")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-fuse-ld=lld -Wl,-platform_version,ios-simulator,26.0,26.0")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-fuse-ld=lld -Wl,-platform_version,ios-simulator,26.0,26.0")

# Linux clang targeting iOS simulator needs --target, not -arch/-mios-simulator-version-min.
# Override CMake's default flags for Apple targets.
set(CMAKE_C_FLAGS_INIT "--target=arm64-apple-ios26.0-simulator -isysroot ${IOS_SDK_PATH} -mios-simulator-version-min=26.0")
set(CMAKE_CXX_FLAGS_INIT "--target=arm64-apple-ios26.0-simulator -isysroot ${IOS_SDK_PATH} -mios-simulator-version-min=26.0")
set(CMAKE_OBJC_FLAGS_INIT "--target=arm64-apple-ios26.0-simulator -isysroot ${IOS_SDK_PATH} -mios-simulator-version-min=26.0")
set(CMAKE_OBJCXX_FLAGS_INIT "--target=arm64-apple-ios26.0-simulator -isysroot ${IOS_SDK_PATH} -mios-simulator-version-min=26.0")

# Find programs on the build machine, not the target.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# For libraries and headers, look only in the SDK.
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# The simulator SDK doesn't have pkg-config files.
set(PKG_CONFIG_EXECUTABLE "/usr/bin/false")

# Tell CMake the SDK is valid for iOS.
set(_CMAKE_OSX_SYSROOT_PATH ${IOS_SDK_PATH})

# Disable CMake's built-in iOS flag logic which adds -arch/-mios-simulator-version-min.
set(CMAKE_OSX_ARCHITECTURES "")
set(CMAKE_OSX_DEPLOYMENT_TARGET "")