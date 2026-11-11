# SPDX-License-Identifier: AGPL-3.0-only

# Minimal Windows cross-compile test: just SDL3 window.

cmake_minimum_required(VERSION 4.0.0)
project(pchat_windows_test LANGUAGES C)

include(FetchContent)

# SDL3 from the project's fork
fetchcontent_declare(sdl3
  GIT_REPOSITORY https://github.com/j4niwzis/SDL
  GIT_TAG 91c08de87b43fdecc824464fcce15499ccbc7101
)
fetchcontent_makeavailable(sdl3)

# Build SDL3 statically
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

add_executable(pchat-windows-test main.c)
target_link_libraries(pchat-windows-test PRIVATE SDL3::SDL3)
target_link_libraries(pchat-windows-test PRIVATE "-lmingw32" "-lSDL3main")