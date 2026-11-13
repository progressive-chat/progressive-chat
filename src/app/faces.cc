// SPDX-License-Identifier: AGPL-3.0-only
// pchat.app.faces -- The faces progressive-chat ships inside its own binary,
// as the window takes a program's own (skiff.platform.fonts): Open Sans,
// regular and semibold, bold text drawn with the semibold rather than one
// thickened. Open Sans is SIL Open Font License 1.1 (fonts/OFL).
//
// Without these a build in a browser draws plates, rows and bubbles and not
// one word: there is no system to ask for fonts instead, and a bubble of
// nothing is all a message comes to. On a system -- desktop, phone -- they
// still make the text the same on every machine, and what Open Sans has no
// glyph for is looked for behind them.
export module pchat.app.faces;

export namespace pchat::app {

struct faces {
  static constexpr unsigned char regular[] = {
#embed "../../fonts/OpenSans-Regular.ttf"
  };
  static constexpr unsigned char semibold[] = {
#embed "../../fonts/OpenSans-SemiBold.ttf"
  };
};

}  // namespace pchat::app
