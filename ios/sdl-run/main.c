# SPDX-License-Identifier: AGPL-3.0-only

// An SDL window on a phone's system: the smallest thing that proves the
// windowing layer works. It opens a window, fills it, and writes a line to
// the log when the first frame is presented.

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

static const char *mark = "PCHAT-IOS-SDL-FIRST-RUN";

int main(int argc, char *argv[]) {
  if (!SDL_Init(SDL_INIT_VIDEO)) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_Init failed: %s", SDL_GetError());
    return 1;
  }

  SDL_Window *window = SDL_CreateWindow("progressive-chat", 320, 480, 0);
  if (!window) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateWindow failed: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  SDL_Renderer *renderer = SDL_CreateRenderer(window, NULL);
  if (!renderer) {
    SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "SDL_CreateRenderer failed: %s", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }

  // First frame: black background, white text.
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  // We don't have TTF here, so just draw a white rectangle where the word would be.
  SDL_FRect rect = {80, 210, 160, 60};
  SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
  SDL_RenderFillRect(renderer, &rect);
  SDL_RenderPresent(renderer);

  SDL_Log("%s: the window is up", mark);

  // Wait a moment so the simulator can snapshot.
  SDL_Delay(1000);

  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}# trigger
