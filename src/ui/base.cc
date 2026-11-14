// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:base -- The measures every part is drawn by, and the few shapes
// the screens are built out of: a plate, a line of text, a pen's icon, a
// circle for a name.
export module pchat.ui:base;

import std;
import skia;
import skiff.scene;
import skiff.nodes;
import pchat.core;
import pchat.config;
import pchat.app.requests;
import :theme;
import :icons;

export namespace pchat::ui {

namespace scene = skiff::scene;
namespace nodes = skiff::nodes;

// The measures, in points: how wide the drawer is, when the window is wide
// enough to keep it beside the messages instead of over them, how tall the
// bar over the list, how far the bubbles may reach.
inline constexpr float drawer_width = 360.0f;
// How much of the messages is left in sight beside a drawer come over them.
// Word by word: `inline` = one shared copy in the program; `constexpr` = the
// number is fixed at build time, never computed at run time; `float` = a
// decimal number; `48.0f` = 48 points, the `f` marks it a float. 48 points of
// the messages stay visible beside the overlaid drawer, and that strip is what
// the reader taps to close the drawer.
inline constexpr float drawer_open_room = 48.0f;
// What the panel keeps between itself and its own parts: its heading further
// in than the field and the rows under it.
inline constexpr float drawer_margin = 20.0f;
// What a name is set in at the head of the panel.
inline constexpr float drawer_heading_size = 26.0f;
// One row of the panel's own -- Tasks, Remote, a new chat, the settings --
// how tall it is, how far it is rounded, and what a row keeps clear at its
// right against what it keeps at its left.
inline constexpr float drawer_row_height = 44.0f;
inline constexpr float drawer_row_size = 15.0f;
inline constexpr float drawer_row_radius = 14.0f;
// Where two rows meet: the corners between them are rounded only barely, so
// the two read as one row cut in two.
inline constexpr float drawer_row_corner = 4.0f;
inline constexpr float drawer_row_gap = 4.0f;
// How much wider a conversation's row keeps its right margin than its left.
inline constexpr float drawer_right_margin = 3.0f;
// How big the mark before a name is, in the panel's own rows and in its field.
inline constexpr float drawer_mark = 18.0f;
inline constexpr float side_by_side_from = 960.0f;
inline constexpr float top_bar_height = 52.0f;
inline constexpr float top_scrim = 180.0f;
inline constexpr float pill_radius = 22.0f;
inline constexpr float row_height = 76.0f;
inline constexpr float avatar = 44.0f;
// No wider than three quarters of the room, and no wider than this, where a
// line is comfortable to read to its end and back.
inline constexpr float bubble_max = 720.0f;
inline constexpr float bubble_radius = 20.0f;
inline constexpr float bubble_corner = 4.0f;
inline constexpr float card_radius = 20.0f;
inline constexpr float send_size = 46.0f;
// What the card written in stands high: what says which model answers and how
// full its window is, over the row the words are written on.
inline constexpr float composer_height = 92.0f;
inline constexpr float gap = 12.0f;
inline constexpr float edge = 12.0f;
// What the body's type is set in, before the reader asks for it larger.
inline constexpr float body_size = 15.0f;

// One thing the program is asked for by a press: the request is kept here,
// whole, and pushed into what the program drains when this is pressed. A
// plate that only asks -- a pill, the shade over the messages a drawer is
// open over -- has nothing else in it.
struct ask_box : scene::Node {
  app::requests *asked = nullptr;
  app::request one{app::toggle_drawer{}};

  [[nodiscard]] bool acceptsInput() const { return true; }
  [[nodiscard]] bool hoverChangesAppearance() const { return true; }
  [[nodiscard]] bool onClick(float, float) {
    if (asked)
      asked->ask(one);
    return true;
  }

  void again(app::requests *sink, app::request asking, scene::Colour colour) {
    asked = sink;
    one = std::move(asking);
    // Its ground said of itself and not of a plate inside it: a plate fills
    // by the corner radius set on it, and none was set, so every round press
    // in the program was painted square. Declared on the node, the ground is
    // painted in the node's own rounded box and what it holds is clipped by
    // the same.
    this->apply({.background = colour});
  }

  void forEachChild(auto &&f) {}
};

// One press with a mark painted in the middle of it: what closes a card, put
// away at the corner of it. The mark is painted after the plate, and what a
// page says was pressed there is the mark: a walk from it climbs to this
// node and no higher, and a node that answers nothing answers nothing for
// the plate either. So this one answers for the plate too, wherever in its
// own box the press came, and nowhere else: the box is no wider than the
// plate (`docs/skiff-notes.md`).
struct press_box : scene::Node {
  ask_box fPlate{};
  nodes::Icon fMark{shape::close(), 0u};

  explicit press_box(nodes::IconShape mark = shape::close())
      : fMark(std::move(mark), 0u) {
    fMark.apply({.width = 18.0f,
                 .height = 18.0f,
                 .anchor = scene::anchor::kCentre,
                 .origin = scene::anchor::kCentre});
  }

  [[nodiscard]] bool acceptsInput() const { return true; }
  [[nodiscard]] bool onClick(float x, float y) { return fPlate.onClick(x, y); }

  void again(app::requests *sink, const scene::Spec &where, app::request asking,
             scene::Colour colour) {
    this->apply(where);
    fPlate.again(sink, std::move(asking), colour);
    fPlate.apply({.fill = true, .cornerRadius = 18.0f});
  }

  void mark_colour(scene::Colour colour) { fMark.setColour(colour); }
  void hide() { this->setVisible(false); }

  void forEachChild(auto &&f) {
    f(fPlate);
    f(fMark);
  }
};

// What the window gives the screens: its size, and what that size asks for.
// Word by word: `struct` = a bundle of related values; `metrics` = the name;
// each `float width/height/scale` is one measurement the window reports every
// frame. `scale = 1.0f` is only the default -- unlike the fixed constants
// above, these are filled in at run time from the real window, so they are
// fields, not `constexpr`.
struct metrics {
  float width = 0.0f;
  float height = 0.0f;
  // Device pixel ratio: 1.0 on a normal screen, 2.0 or 3.0 on a high-DPI phone.
  // This is why it is not a constant -- it changes with the screen it runs on.
  float scale = 1.0f;
  // Wide enough for the drawer to stay beside the messages: a phone keeps one
  // at a time.
  [[nodiscard]] bool wide() const noexcept {
    return width >= side_by_side_from;
  }
  [[nodiscard]] float drawer_breadth() const noexcept {
    return std::min(drawer_width, std::max(width * 0.32f, 280.0f));
  }
};

// ---- the shapes --------------------------------------------------------

// A plate: one colour rounded, a bubble, a pill, a card.
// Word by word: `[[nodiscard]]` = the caller must use the returned value;
// `inline` = one shared copy; `nodes::Box<>` = the rectangle node type from
// skiff (the drawing library); `scene::Spec` = a struct of layout hints (size,
// position, corner radius); `scene::Colour` = an RGBA colour. The body just
// asks skiff to build a Box from the spec and colour it -- so `plate` is a one
// line factory for any rounded rectangle the UI needs.
[[nodiscard]] inline nodes::Box<> plate(const scene::Spec &spec,
                                        scene::Colour colour) {
  return scene::make<nodes::Box>(spec, colour);
}
// How many tokens something was worth, said as it is read: its thousands
// parted, because eleven of them in a row is not a number read at a glance.
[[nodiscard]] inline std::string tokens_said(std::uint32_t tokens) {
  const std::string digits = std::to_string(tokens);
  std::string out;
  for (std::size_t at = 0; at < digits.size(); ++at) {
    if (at != 0 && (digits.size() - at) % 3 == 0)
      out += ",";
    out += digits[at];
  }
  return out;
}
// The same number said short, as the bar over the messages says it: its
// thousands and one figure after them -- `12.4k` -- because the bar has one
// word of width to spare for what this chat has been worth.
[[nodiscard]] inline std::string tokens_short(std::uint32_t tokens) {
  if (tokens < 1'000)
    return std::to_string(tokens);
  if (tokens < 1'000'000) {
    const std::uint32_t tenths = (tokens / 100) % 10;
    return std::to_string(tokens / 1'000) +
           (tenths ? "." + std::to_string(tenths) : "") + "k";
  }
  const std::uint32_t tenths = (tokens / 100'000) % 10;
  return std::to_string(tokens / 1'000'000) +
         (tenths ? "." + std::to_string(tenths) : "") + "M";
}
// The clock as it is read in this window: the hour of the day and the minute
// in it, out of a moment kept as it is in the model. No zone by itself to
// ask, because a chat is written in its reader's zone.
[[nodiscard]] inline std::string clock_of(moment at) {
  const std::int64_t minutes = (at / 60'000) % (24 * 60);
  const std::int64_t hour = minutes / 60;
  const std::int64_t minute = minutes % 60;
  std::string out;
  out += hour < 10 ? "0" : "";
  out += std::to_string(hour);
  out += ":";
  out += minute < 10 ? "0" : "";
  out += std::to_string(minute);
  return out;
}
// A line of text, of a size by itself, bold or not.
[[nodiscard]] inline nodes::Text line(std::string value, float size,
                                      scene::Colour colour, bool bold = false,
                                      const scene::Spec &spec = {}) {
  return scene::make<nodes::Text>(spec, std::move(value), size, colour, bold);
}
// An icon drawn with a pen, in points from the middle of its box.
[[nodiscard]] inline nodes::Icon
pen(const scene::Spec &spec, nodes::IconShape shape, scene::Colour colour) {
  return scene::make<nodes::Icon>(spec, std::move(shape), colour);
}
// A circle of one colour, its own size, for what stands in the icon's place.
// Word by word: `scene::Spec made = spec;` copies the incoming layout hints;
// `std::max(a, b)` takes the larger of the two; `spec.width.value_or(avatar)`
// means "the width if one was given, otherwise `avatar` (44 points)". Setting
// the corner radius to half the larger side rounds a square into a circle, so
// this builds a filled circle of the given colour.
[[nodiscard]] inline nodes::Box<> round(const scene::Spec &spec,
                                        scene::Colour colour) {
  scene::Spec made = spec;
  made.cornerRadius =
      std::max(spec.width.value_or(avatar), spec.height.value_or(avatar));
  return scene::make<nodes::Box>(made, colour);
}

} // namespace pchat::ui
