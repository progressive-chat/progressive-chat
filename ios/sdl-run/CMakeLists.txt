# SPDX-License-Identifier: AGPL-3.0-only

# The SDL window for the first run: built by CMake, on Linux, for the
# simulator. The toolchain is given from the outside.

cmake_minimum_required(VERSION 4.3.4)
project(pchat_ios_sdl LANGUAGES C CXX OBJC)

# SDL3 from the project's fork (has the iOS main callbacks and NativeActivity).
fetchcontent_declare(sdl3
  GIT_REPOSITORY https://github.com/j4niwzis/SDL
  GIT_TAG 91c08de87b43fdecc824464fcce15499ccbc7101
  SOURCE_DIR "${CMAKE_BINARY_DIR}/sdl3-src")
fetchcontent_makeavailable(sdl3)

# Build SDL3 statically, no extra drivers we don't need.
set(SDL_SHARED OFF)
set(SDL_STATIC ON)
set(SDL_INSTALL OFF)
set(SDL_TEST OFF)
set(SDL_RENDER_METAL OFF)
set(SDL_RENDER_VULKAN OFF)
set(SDL_RENDER_DIRECT3D11 OFF)
set(SDL_RENDER_DIRECT3D12 OFF)
set(SDL_RENDER_PS2 OFF)
set(SDL_RENDER_PSP OFF)
set(SDL_JOYSTICK_HIDAPI OFF)
set(SDL_HIDAPI OFF)
set(SDL_SENSOR OFF)
set(SDL_POWER OFF)
set(SDL_FILESYSTEM OFF)
set(SDL_LOCALE OFF)
set(SDL_CAMERA OFF)
set(SDL_DIALOG OFF)
set(SDL_CLIPBOARD ON)
# iOS needs the main callbacks from the fork.
set(SDL_MAIN_CALLBACKS ON)

add_subdirectory("${sdl3_SOURCE_DIR}" "${sdl3_BINARY_DIR}")

add_executable(pchat-sdl ios/sdl-run/main.c)
target_link_libraries(pchat-sdl PRIVATE SDL3::SDL3)
target_link_libraries(pchat-sdl PRIVATE "-framework Foundation" "-framework UIKit" "-framework CoreGraphics" "-framework QuartzCore")