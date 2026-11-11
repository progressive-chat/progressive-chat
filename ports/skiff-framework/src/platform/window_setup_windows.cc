// SPDX-License-Identifier: AGPL-3.0-only
// Windows font path for skiff-framework.
// Used when building for Windows target.

export module skiff.platform.window_setup;
export namespace skiff::platform::window_setup {
inline constexpr const char* fonts = "C:/Windows/Fonts";
inline void graphics() {}
}