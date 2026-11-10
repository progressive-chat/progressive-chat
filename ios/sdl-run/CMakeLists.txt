# SPDX-License-Identifier: AGPL-3.0-only

# The SDL window for the first run: built by CMake, on Linux, for the
# simulator. The toolchain is given from the outside.

cmake_minimum_required(VERSION 3.31)
project(pchat_ios_sdl LANGUAGES C CXX OBJC)

# SDL3 source is cloned by the workflow into sdl3-src.
add_subdirectory(sdl3-src)

# Build SDL3 statically, no extra drivers we don't need.
set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_INSTALL OFF CACHE BOOL "" FORCE)
set(SDL_TEST OFF CACHE BOOL "" FORCE)
set(SDL_RENDER_METAL OFF CACHE BOOL "" FORCE)
set(SDL_RENDER_VULKAN OFF CACHE BOOL "" FORCE)
set(SDL_RENDER_DIRECT3D11 OFF CACHE BOOL "" FORCE)
set(SDL_RENDER_DIRECT3D12 OFF CACHE BOOL "" FORCE)
set(SDL_RENDER_PS2 OFF CACHE BOOL "" FORCE)
set(SDL_RENDER_PSP OFF CACHE BOOL "" FORCE)
set(SDL_JOYSTICK_HIDAPI OFF CACHE BOOL "" FORCE)
set(SDL_HIDAPI OFF CACHE BOOL "" FORCE)
set(SDL_SENSOR OFF CACHE BOOL "" FORCE)
set(SDL_POWER OFF CACHE BOOL "" FORCE)
set(SDL_FILESYSTEM OFF CACHE BOOL "" FORCE)
set(SDL_LOCALE OFF CACHE BOOL "" FORCE)
set(SDL_CAMERA OFF CACHE BOOL "" FORCE)
set(SDL_DIALOG OFF CACHE BOOL "" FORCE)
set(SDL_CLIPBOARD ON CACHE BOOL "" FORCE)
# iOS needs the main callbacks from the fork.
set(SDL_MAIN_CALLBACKS ON CACHE BOOL "" FORCE)

add_executable(pchat-sdl main.c)
target_link_libraries(pchat-sdl PRIVATE SDL3::SDL3)
target_link_libraries(pchat-sdl PRIVATE "-framework Foundation" "-framework UIKit" "-framework CoreGraphics" "-framework QuartzCore")