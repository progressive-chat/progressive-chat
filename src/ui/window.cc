// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:window -- One window: the drawer of conversations down one side,
// the messages beside it under the bar that floats over them, the card to
// write in at the foot, and the settings page over the whole of it when it is
// opened.
//
// The parts are made once, with the window, and filled in again by `again`:
// the tree's shape never changes, only what its parts say and where they
// stand, as the window's own size asks.
export module pchat.ui:window;

import std;
import skiff.scene;
import skiff.nodes;
import pchat.core;
import pchat.config;
import pchat.app.requests;
import :theme;
import :base;
import :drawer;
import :top_bar;
import :message_list;
import :composer;
import :settings;
import :doc;
import :prompt;

export namespace pchat::ui {

namespace scene = skiff::scene;
namespace nodes = skiff::nodes;

// What of the program is open, as one thing instead of six bools in a row:
// asked for in that order, a call read like `again(look, root, shown, how,
// query, true, false, false, true, false)`. Which are open is the program's
// own state; what the window does with them is said here, in words.
struct what_is_open {
  bool settings = false;
  bool design = false;
  // What every answerer in the conversation open is told before anything is
  // said to it, read and written whole.
  bool prompt = false;
  // What the bar's three dots hold.
  bool more = false;
  // What the card's own three dots hold.
  bool editor_menu = false;
  // The card of faces, opened by the face beside the clip.
  bool faces = false;
  // The drawer, over the messages where the room is too narrow for it to
  // stand beside them.
  bool drawer = false;
  friend bool operator==(const what_is_open &, const what_is_open &) = default;
};

// Everything the program shows, as one node a Scene can hold. Made with the
// requests it hands its controls: a control asks, the program answers between
// frames.
struct app_window : scene::Node {
  nodes::Box<> fPaper{0u};
  nodes::Box<> fMessages{0u};
  ask_box fShade{};
  // What puts away what three dots opened, wherever they stand: nothing
  // painted and everything pressed, taking the whole room while either card
  // is out and none of it when none is. It stands here, above the messages
  // and below the bar and the card: a press is taken by what was painted last
  // over the place. The messages, the list and the drawer hand their presses
  // to it; the bar, the card and every row either opened are painted after it
  // and keep theirs. So a card opened over the room is put away by a press
  // anywhere below it, while a press on the card, its three dots or a row
  // behind them does what that one says.
  ask_box fAway{};
  drawer_screen fDrawer{360.0f};
  top_bar fTop{};
  nodes::ScrollContainer<message_screen> fList{message_screen{}};
  composer fComposer{};
  settings_screen fSettings{};
  doc_screen fDesign{};
  prompt_screen fPrompt{};

  app_window() {
    this->apply({.fill = true});
    fPaper.apply({.fill = true});
    fMessages.apply({.background = 0u});
    // A conversation is read at its foot: messages show from their last one
    // and stay there as they grow, until the reader scrolls away, which the
    // list keeps instead.
    fList.scrollToEnd(false);
  }

  // What the messages are called, from the model: the title the bar shows.
  [[nodiscard]] static std::string title_of(const chats_root &root,
                                            const std::string &opened) {
    for (const conversation &chat : root.conversations)
      if (chat.id.value == opened)
        return chat.title;
    return std::string{};
  }

  // Everything drawn again from the model, for this size of window.
  void again(const paints &look, const chats_root &root, const looks &shown,
             const asking &how, const std::string &query, bool dark,
             const what_is_open &open,
             const std::vector<std::string> &faces_lately, float width,
             float height, app::requests *asked) {
    // The conversation the window has open, as it is named: none, where none
    // is open.
    const std::string opened = root.opened ? root.opened->value : std::string{};
    const bool wide = width >= side_by_side_from;
    const float breadth =
        std::min(drawer_width, std::max(width * 0.32f, 280.0f));
    const bool in_chat = wide || !open.drawer;
    const float column = in_chat ? width - (wide ? breadth : 0.0f) : width;
    const bool aside = wide || open.drawer;
    // The messages are drawn under the drawer where the drawer covers them
    // too: what is left beside it is read there, and pressing it puts the
    // drawer away again.
    const float left = wide ? breadth : 0.0f;
    fPaper.setColour(look.paper);
    // The messages' own ground, beside the drawer. A size in pixels, not a
    // share: `relativeSize` is none here, or the number reads as parts of
    // one.
    fMessages.apply({.anchor = scene::anchor::kTopLeft,
                     .x = left,
                     .y = 0.0f,
                     .relativeSize = scene::axes::kNone,
                     .width = column,
                     .height = height,
                     .background = look.paper,
                     .visible = true});
    // The drawer: beside the messages when the window is wide, over them when
    // not. Over them it leaves a strip of them in sight; the reader presses
    // that strip to put it away again.
    const float drawer_breadth =
        wide ? breadth
             : std::min(width, std::max(breadth, width - drawer_open_room));
    fDrawer.apply({.anchor = scene::anchor::kTopLeft,
                   .x = 0.0f,
                   .y = 0.0f,
                   .relativeSize = scene::axes::kNone,
                   .width = drawer_breadth,
                   .height = height,
                   .visible = aside,
                   .background = look.drawer});
    if (aside) {
      fDrawer.apply({.anchor = scene::anchor::kTopLeft,
                     .x = 0.0f,
                     .y = 0.0f,
                     .relativeSize = scene::axes::kNone,
                     .width = drawer_breadth,
                     .height = height,
                     .visible = true,
                     .background = look.drawer});
      fDrawer.again(look, shown, root, opened, query, asked);
    }
    // The bar, over the top of the messages.
    fTop.apply({.anchor = scene::anchor::kTopLeft,
                .x = left,
                .y = 0.0f,
                .relativeSize = scene::axes::kNone,
                .width = column,
                .height = top_scrim,
                .visible = true});
    // The pill that opens the drawer shows where a drawer can be opened: a
    // window wide enough to keep it beside the messages has none to open.
    const conversation *open_chat = nullptr;
    for (const conversation &one : root.conversations)
      if (one.id.value == opened)
        open_chat = &one;
    // Which answerers answer in the conversation open: what the settings say
    // of them is said of this one chat, no other.
    std::vector<std::string> answering;
    if (open_chat)
      answering = open_chat->models;
    // The bar says what the chat has been worth; the card under it names the
    // model answering in it and how full its window this chat has made it.
    fTop.again(asked, look, !wide, !wide, open.more, title_of(root, opened),
               open_chat);
    // Over what the messages were, where the drawer covers them: pressing it
    // puts the drawer away again. What puts a card of three dots away -- the
    // bar's or the card's -- each part keeps to itself: see
    // `pchat.ui:composer`, what stands behind its three dots.
    fShade.apply({.anchor = scene::anchor::kTopLeft,
                  .x = 0.0f,
                  .y = 0.0f,
                  .relativeSize = scene::axes::kNone,
                  .width = width,
                  .height = height,
                  .visible = !wide && open.drawer});
    fShade.again(asked, app::toggle_drawer{}, look.scrim);
    // The messages, in what scrolls, between the bar and the card.
    auto &messages = std::get<0>(fList.fChildren);
    // What answered in this chat, said where About asks: the room itself
    // never names it, so the rows are given it only to keep it for that one
    // line. Every model that answers here, as one string: the About row names
    // them together, the bubbles name each apart.
    std::string answerers;
    if (open_chat)
      for (const std::string &one : open_chat->models) {
        if (!answerers.empty())
          answerers += ", ";
        answerers += one;
      }
    messages.again(asked, look, root, opened, column,
                   std::string_view(answerers));
    fList.apply(
        {.anchor = scene::anchor::kTopLeft,
         .x = left,
         .y = top_bar_height,
         .relativeSize = scene::axes::kNone,
         .width = column,
         .height = std::max(
             height - top_bar_height - composer_height - 2.0f * edge, 40.0f),
         .visible = true});
    // The card at the foot.
    fComposer.apply({.anchor = scene::anchor::kBottomLeft,
                     .origin = scene::anchor::kBottomLeft,
                     .x = left,
                     .y = 0.0f,
                     .relativeSize = scene::axes::kNone,
                     .width = column,
                     .height = composer_height + 2.0f * edge,
                     .visible = true});
    if (open_chat)
      fComposer.again(look, *open_chat, how, asked, open.editor_menu,
                      open.faces, faces_lately);
    // Any of the three cards, put away by a press on anything below them: the
    // bar's three dots first where two are out at once, then the one written
    // into, on a second press of the same kind.
    const bool card_out = open.more || open.editor_menu || open.faces;
    fAway.apply({.anchor = scene::anchor::kTopLeft,
                 .x = 0.0f,
                 .y = 0.0f,
                 .width = width,
                 .height = height,
                 .relativeSize = scene::axes::kNone,
                 .visible = card_out});
    if (card_out)
      fAway.again(asked,
                  open.more ? app::request{app::toggle_more{}}
                  : open.editor_menu
                      ? app::request{app::toggle_editor_menu{}}
                      : app::request{app::toggle_faces{}},
                  look.away);
    // The settings page, over everything, when it is opened. The words the
    // keeping row says beside its name: what the model carries now, which is
    // what the store holds while that switch is on (it writes every change),
    // and what the row stands next to while it is off.
    if (open.settings) {
      std::string kept{"Kept here: " +
                       std::to_string(root.conversations.size()) +
                       " conversations, " +
                       std::to_string(root.messages.size()) + " messages"};
      fSettings.again(asked, look, shown, how, kept, answering, dark, width,
                      height);
    }
    else
      fSettings.hide();
    // The design shown in the room when opened: over everything, a page of
    // words, and put away again whole.
    if (open.design)
      fDesign.again(asked, look, std::min(width, 760.0f), height);
    else
      fDesign.hide();
    // What every model is told here when opened: over everything, put away
    // again with what was written in it kept.
    if (open.prompt)
      fPrompt.again(asked, look, open_chat, width, height);
    else
      fPrompt.hide();
  }

  void forEachChild(auto &&f) {
    f(fPaper);
    f(fMessages);
    f(fShade);
    f(fDrawer);
    f(fList);
    f(fAway);
    f(fTop);
    f(fComposer);
    f(fSettings);
    f(fDesign);
    f(fPrompt);
  }
};

// The type a Scene holds: this one's root.
using window_type = app_window;

} // namespace pchat::ui
