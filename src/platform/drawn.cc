// SPDX-License-Identifier: AGPL-3.0-only
// pchat.platform.drawn -- Whether this machine can be drawn through its GPU:
// asked once, before the window is made, of a window nobody will ever see.
module;
#include <SDL3/SDL.h>

export module pchat.platform.drawn;

import std;

export namespace pchat::platform {

// With no GL -- no video driver, a display server that was never started,
// WebGL turned off in the page -- Skia asks GL what it has, is answered with
// a null pointer, and reads through it: the program dies before its first
// frame and leaves only a crash. Probed here first, so the window can be told
// to draw in software, which is slower and still a window.
//
// A phone is not probed: it always has GL, and probing takes a window SDL
// will not always give back.
bool gpu_can_be_had() {
#if defined(__ANDROID__) || defined(__EMSCRIPTEN__)
  return true;
#else
  if (!SDL_InitSubSystem(SDL_INIT_VIDEO))
    return false;
  SDL_Window *seen_by_nobody =
      SDL_CreateWindow("", 1, 1, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN);
  if (!seen_by_nobody)
    return false;
  const bool there =
      (SDL_GetWindowFlags(seen_by_nobody) & SDL_WINDOW_OPENGL) != 0u;
  SDL_GLContext held =
      there ? SDL_GL_CreateContext(seen_by_nobody) : nullptr;
  if (held)
    SDL_GL_DestroyContext(held);
  SDL_DestroyWindow(seen_by_nobody);
  return held != nullptr;
#endif
}

} // namespace pchat::platform
