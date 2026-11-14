// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:top_bar -- What floats over the top of the messages: a band of the
// paper's own colour fading out downwards, the conversation's name in the
// middle, the pill that opens the drawer on one side, and on the other the
// models that answer in this conversation, one bubble each, beside the
// presses that add one, say what every one of them is told, take this
// conversation into a new chat, share it, and begin an empty one.
//
// With no room for those presses to stand as themselves, four of them stand
// behind three dots and come out of them: what is narrow is a window too
// narrow for the lot, not a phone in particular.
export module pchat.ui:top_bar;

import std;
import skiff.scene;
import skiff.nodes;
import pchat.core;
import pchat.config;
import pchat.app.requests;
import :theme;
import :base;
import :icons;

export namespace pchat::ui {

namespace scene = skiff::scene;
namespace nodes = skiff::nodes;

// How many answerers one conversation may have at once, how wide a bubble is,
// and how far apart two of anything in the bar stand.
inline constexpr std::size_t max_answerers = 4u;
inline constexpr float answerer_bubble_width = 176.0f;
inline constexpr float answerer_bubble_height = 32.0f;
inline constexpr float bar_press_width = 36.0f;
inline constexpr float bar_press_step = 40.0f;
inline constexpr float bar_gap = 8.0f;
// What the three dots hold, and how far under them their rows hang.
inline constexpr std::size_t max_bar_more = 3u;
inline constexpr float more_row_height = 44.0f;
inline constexpr float more_card_width = 208.0f;

// One model that answers in this conversation: its name alone. What this
// conversation took of the window is counted once, in the editor above:
// a three-model chat would say that number three times in its bubbles.
struct answerer_bubble : scene::Node {
  nodes::Box<> fPlate{0u};
  nodes::Text fName{"", 12.5f, 0u, true};

  answerer_bubble() {
    fPlate.apply({.fill = true});
    fName.apply({.anchor = scene::anchor::kCentreLeft,
                 .origin = scene::anchor::kCentreLeft,
                 .x = 12.0f});
  }

  void again(const paints &look, const scene::Spec &where,
             std::string_view name) {
    this->apply(where);
    // The bubble is rounded as the spec brought here says, and its plate is
    // what is seen: only the radius is taken, nothing else of where it
    // stands, which the plate already has.
    if (where.cornerRadius)
      fPlate.apply({.cornerRadius = *where.cornerRadius});
    fPlate.setColour(look.search_bar);
    fName.setText(std::string(name));
    fName.setColour(look.text);
    this->setVisible(true);
  }

  void hide() { this->setVisible(false); }

  void forEachChild(auto &&f) {
    f(fPlate);
    f(fName);
  }
};

// One press in the bar: its plate, and the mark said in it. Every press in
// the top is this -- the same plate, the same size, one to the next -- and
// each says what it asks for.
struct bar_press : scene::Node {
  ask_box fPlate{};
  nodes::Icon fMark{shape::close(), 0u};

  bar_press() {
    // As wide and as tall as what it presses, and no wider. Set to the bar's
    // full breadth instead, each of these is painted after the presses to its
    // left and over them: what the page says was pressed is what it painted
    // last, so the three to the left would answer a press nowhere in the bar.
      // What is set instead is a separate box, standing where its plate stood
    // and no further, the way a row behind the three dots already gives its
    // own size.
    fMark.apply({.width = 18.0f,
                 .height = 18.0f,
                 .anchor = scene::anchor::kCentre,
                 .origin = scene::anchor::kCentre});
  }

  void again(app::requests *sink, const paints &look, nodes::IconShape mark,
             float inset, app::request asking, bool left = false) {
    this->apply({.anchor = left ? scene::anchor::kTopLeft
                                : scene::anchor::kTopRight,
                 .origin = left ? scene::anchor::kTopLeft
                                : scene::anchor::kTopRight,
                 .x = left ? inset : -inset,
                 .y = (top_bar_height - bar_press_width) / 2.0f,
                 .relativeSize = scene::axes::kNone,
                 .width = bar_press_width,
                 .height = bar_press_width,
                 .visible = true});
    fPlate.again(sink, std::move(asking), look.search_bar);
    fPlate.apply({.fill = true, .cornerRadius = bar_press_width / 2.0f});
    fMark.setShape(std::move(mark));
    fMark.setColour(look.text);
    fMark.apply({.anchor = scene::anchor::kCentre,
                 .origin = scene::anchor::kCentre,
                 .relativeSize = scene::axes::kNone,
                 .width = 18.0f,
                 .height = 18.0f,
                 .visible = true});
  }

  // The mark is painted over the middle of its plate, and a press on it goes
  // no further than what holds it: the page says the mark was pressed and a
  // walk from it climbs to this node and no higher. This node answers for the
  // plate too, wherever in its box it was pressed, and nowhere else: it is no
  // wider than the plate.
  [[nodiscard]] bool acceptsInput() const { return true; }
  [[nodiscard]] bool onClick(float x, float y) { return fPlate.onClick(x, y); }

  void hide() { this->setVisible(false); }

  void forEachChild(auto &&f) {
    f(fPlate);
    f(fMark);
  }
};

// One row behind the three dots: named, and pressed where the name is.
struct more_row : scene::Node {
  ask_box fPlate{};
  nodes::Text fLabel{"", 14.0f, 0u};

  more_row() {
    fPlate.apply({.fill = true});
    fLabel.apply({.anchor = scene::anchor::kCentreLeft,
                  .origin = scene::anchor::kCentreLeft,
                  .x = 14.0f});
  }

  void again(app::requests *sink, const paints &look, float top,
             std::string_view label, app::request asking) {
    this->apply({.anchor = scene::anchor::kTopLeft,
                 .x = 0.0f,
                 .y = top,
                 .relativeSize = scene::axes::kNone,
                 .width = more_card_width,
                 .height = more_row_height,
                 .visible = true});
    fPlate.again(sink, std::move(asking), look.drawer);
    fLabel.setText(std::string(label));
    fLabel.setColour(look.text);
  }

  void hide() { this->apply({.visible = false}); }

  void forEachChild(auto &&f) {
    f(fPlate);
    f(fLabel);
  }
};

// What the three dots open: three rows under them, in a card of the drawer's
// own colour, over everything and put away by one press of what opened it or
// of what it holds.
struct more_menu : scene::Node {
  nodes::Box<> fCard{0u};
  std::array<more_row, max_bar_more> fRows;

  more_menu() {
    fCard.apply({.fill = true, .cornerRadius = 16.0f});
    for (more_row &one : fRows)
      one.hide();
    this->setVisible(false);
  }

  void hide() {
    this->setVisible(false);
    for (more_row &one : fRows)
      one.hide();
  }

  void again(app::requests *sink, const paints &look, float inset,
             const conversation_id &chat) {
    this->setVisible(true);
    this->apply({.anchor = scene::anchor::kTopRight,
                 .origin = scene::anchor::kTopRight,
                 .x = -inset,
                 .y = top_bar_height - 4.0f,
                 .relativeSize = scene::axes::kNone,
                 .width = more_card_width,
                 .height = max_bar_more * more_row_height + 2.0f * edge,
                 .cornerRadius = 16.0f,
                 .visible = true});
    fCard.setColour(look.drawer);
    fRows[0].again(sink, look, edge, "System prompt",
                   app::open_system_prompt{.chat = chat});
    fRows[1].again(sink, look, edge + more_row_height, "Fork conversation",
                   app::fork_conversation{.chat = chat});
    fRows[2].again(sink, look, edge + 2.0f * more_row_height, "Share",
                   app::share_conversation{.chat = chat});
  }

  void forEachChild(auto &&f) {
    f(fCard);
    for (more_row &one : fRows)
      f(one);
  }
};

// The bar itself: its fading band behind the pills, the name in the middle,
// the pill that opens the drawer at one side, where a drawer can be opened (a
// wide window has none), and nothing else: no search by itself, the drawer
// is what searches.
struct top_bar : scene::Node {
  nodes::Box<> fScrim{0u};
  bar_press fMenu{};
  bar_press fNew{};
  std::array<answerer_bubble, max_answerers> fAnswerers;
  bar_press fSystem{};
  bar_press fFork{};
  bar_press fShare{};
  bar_press fMore{};
  more_menu fMoreMenu{};
  nodes::Text fTokens{"", 11.0f, 0u};
  nodes::Text fTitle{"", 17.0f, 0u, true};

  top_bar() {
    this->apply({.fillX = true, .height = top_scrim});
    fScrim.apply({.fill = true});
    // Its two colours are the scheme's paper and scrim, handed down and given
    // to it every frame (below): none are of this part's making, so none are
    // written twice. A chat made new, and what this one has been worth: the
    // one pressed at the right, the other read beside it.
    fTokens.apply({.anchor = scene::anchor::kTopRight,
                   .origin = scene::anchor::kCentreRight,
                   .x = -(edge + bar_press_width + bar_gap),
                   .y = top_bar_height / 2.0f});
    fTitle.apply({.anchor = scene::anchor::kTopCentre,
                  .origin = scene::anchor::kTopCentre,
                  .y = 14.0f});
  }

  void again(app::requests *sink, const paints &look, bool menu, bool narrow,
             bool more, std::string_view title, const conversation *chat) {
    fScrim.setColour(look.paper);
    fScrim.apply({.fill = true,
                  .gradient = scene::Gradient{look.paper, look.scrim_top}});
    if (menu)
      fMenu.again(sink, look, shape::menu(), edge, app::toggle_drawer{}, true);
    else
      fMenu.hide();
    fNew.again(sink, look, shape::plus(), edge, app::new_chat{});
    fTitle.setText(std::string(title));
    fTitle.setColour(look.text);
    // Every press stands where the last left room, walking in from the right:
    // what the answerers are told, this conversation taken into one of its
    // own, given away, and behind three dots where the room is too narrow for
    // the three to stand. No plus is among them: the only plus in this part
    // of the screen begins a chat, and which models answer this conversation
    // is said in the settings.
    float inset = edge + bar_press_width + bar_gap;
    if (narrow) {
      fMore.again(sink, look, shape::more(), inset, app::toggle_more{});
      fSystem.hide();
      fFork.hide();
      fShare.hide();
    } else {
      fSystem.again(sink, look, shape::system_prompt(), inset,
                    app::open_system_prompt{
                        .chat = chat ? chat->id : conversation_id{}});
      fFork.again(sink, look, shape::fork(), inset + bar_press_step,
                  app::fork_conversation{.chat = chat ? chat->id
                                                       : conversation_id{}});
      fShare.again(sink, look, shape::share(), inset + 2.0f * bar_press_step,
                   app::share_conversation{.chat = chat ? chat->id
                                                         : conversation_id{}});
      fMore.hide();
    }
    if (narrow && more)
      fMoreMenu.again(sink, look, edge,
                      chat ? chat->id : conversation_id{});
    else
      fMoreMenu.hide();
    // What answers in this chat, one bubble each, named and nothing more.
    // Nothing said of any of them where no conversation is open.
    const std::size_t answerers =
        chat ? std::min(chat->models.size(), max_answerers) : 0u;
    const float left_of_bubbles =
        inset + max_bar_more * bar_press_step + bar_gap;
    for (std::size_t at = 0; at < max_answerers; ++at) {
      if (at >= answerers) {
        fAnswerers[at].hide();
        continue;
      }
      const std::size_t from_right = answerers - 1u - at;
      fAnswerers[at].again(
          look,
          {.anchor = scene::anchor::kTopRight,
           .origin = scene::anchor::kTopRight,
           .x = -(left_of_bubbles +
                  static_cast<float>(from_right) *
                      (answerer_bubble_width + bar_gap)),
           .y = (top_bar_height - answerer_bubble_height) / 2.0f,
           .relativeSize = scene::axes::kNone,
           .width = answerer_bubble_width,
           .height = answerer_bubble_height,
           .cornerRadius = answerer_bubble_height / 2.0f,
           .visible = true},
          said_as_a_model(chat->models[at]));
    }
    // What this whole conversation has been worth, written once beside them,
    // not in each bubble: the bar named it before any bubble held it.
    fTokens.setText(chat ? tokens_short(chat->tokens) + " tokens"
                         : std::string{});
    fTokens.setColour(look.faint);
    fTokens.apply({.anchor = scene::anchor::kTopRight,
                   .origin = scene::anchor::kCentreRight,
                   .x = -(left_of_bubbles +
                          static_cast<float>(answerers) *
                              (answerer_bubble_width + bar_gap) +
                          bar_gap),
                   .y = top_bar_height / 2.0f,
                   .relativeSize = scene::axes::kNone,
                   .visible = true});
  }

  void forEachChild(auto &&f) {
    f(fScrim);
    f(fMenu);
    f(fTitle);
    for (answerer_bubble &one : fAnswerers)
      f(one);
    f(fSystem);
    f(fFork);
    f(fShare);
    f(fMore);
    f(fMoreMenu);
    f(fNew);
    f(fTokens);
  }
};

} // namespace pchat::ui
