// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:composer -- The card at the foot of the messages: which model
// answers in this chat and how full its window is, said over the field always;
// one field to write in, a clip for what is attached, a face for emoji, and
// the round button that sends, which only lights up once something has been
// written.
export module pchat.ui:composer;

import std;
import skiff.scene;
import skiff.nodes;
import skiff.widgets.textarea;
import pchat.core;
import pchat.config;
import pchat.logic.drafts;
import pchat.app.requests;
import :theme;
import :base;
import :icons;

export namespace pchat::ui {

namespace scene = skiff::scene;
namespace nodes = skiff::nodes;
namespace widgets = skiff::widgets;

// What the field is told through: kept by the composer, and read as Enter is
// pressed in it. The field is made once -- with nothing of a conversation in
// it -- so what it asks for goes by this, not by anything of its own.
struct message_sink {
  app::requests *asked = nullptr;
  conversation_id chat;
  // Said once something written in the field has been sent from it, and
  // said false again as soon as the field is left empty: nothing keeps a
  // message it has already given away, and the two ways out of the field --
  // Enter pressed in it, and the round button beside it -- leave it the
  // same.
  bool sent = false;
};
// What Enter in the field asks for: the message sent, of the conversation the
// sink has been given.
struct message_given {
  std::shared_ptr<message_sink> sink;
  void operator()(std::string_view text) const {
    if (sink && sink->asked && !text.empty()) {
      sink->asked->ask(app::send_message{sink->chat, std::string(text)});
      sink->sent = true;
    }
  }
};

// The ring that says how full a model's window is: its track whole underneath
// in the outline's colour, and over it the share of it this chat has taken,
// from twelve o'clock and clockwise.
[[nodiscard]] inline nodes::IconShape
context_ring(float used, scene::Colour track, scene::Colour filled) {
  IconShape out{{{marks::circle{0.0f, 0.0f, 7.0f}, 2.5f, false, track}}};
  if (used > 0.0f)
    out.marks.push_back(
        {marks::arc{-7.0f, -7.0f, 7.0f, 7.0f, -90.0f, used * 360.0f}, 2.5f,
         false, filled});
  return out;
}

// One press with a mark painted in the middle of it, standing for the round
// button beside the field: what the mark is for is not kept here -- it is
// read out of the field at the press, which is why this is its own node and
// not the press-box the other marks of its kind share (`src/ui/base.cc`).
struct send_circle : scene::Node {
  // A plate that takes no press of its own -- a walk up from anywhere in the
  // button, mark or not, reaches this node and is answered here.
  nodes::Box<> fPlate{0u};
  nodes::Icon fMark{shape::send(), 0u};
  widgets::TextArea<message_given> *field = nullptr;
  std::shared_ptr<message_sink> sink;

  send_circle() {
    fMark.apply({.width = 22.0f,
                 .height = 22.0f,
                 .anchor = scene::anchor::kCentre,
                 .origin = scene::anchor::kCentre});
  }

  [[nodiscard]] bool acceptsInput() const { return true; }
  [[nodiscard]] bool hoverChangesAppearance() const { return true; }
  // Pressed: what Enter pressed in the field asks for, asked the same way,
  // which is nothing at all while there is nothing written in it.
  [[nodiscard]] bool onClick(float, float) {
    if (field && sink)
      message_given{sink}(field->plainText());
    return true;
  }

  void again(const scene::Spec &where, scene::Colour colour,
             scene::Colour mark) {
    this->apply(where);
    fPlate.apply({.fill = true, .cornerRadius = send_size / 2.0f});
    fPlate.setColour(colour);
    fMark.setColour(mark);
  }

  void forEachChild(auto &&f) {
    f(fPlate);
    f(fMark);
  }
};

// What the three dots open, and how much room one row of it has: as wide as
// what the bar's own three dots open, and one line less high than a row of
// the drawer.
inline constexpr float editor_menu_width = 208.0f;
inline constexpr float editor_row_height = 40.0f;
inline constexpr std::size_t editor_rows = 3u;
// How far the three dots stand from the card's right edge, and how much room
// they leave the ring beside them for.
inline constexpr float dots_size = 22.0f;
inline constexpr float dots_room = dots_size + 14.0f;

// One row behind the card's three dots: what it is called, what it stands at
// now said at its right, and pressed anywhere the row is.
struct editor_row : scene::Node {
  ask_box fPlate{};
  nodes::Text fLabel{"", 14.0f, 0u};
  nodes::Text fNow{"", 14.0f, 0u};

  editor_row() {
    fPlate.apply({.fill = true});
    fLabel.apply({.anchor = scene::anchor::kCentreLeft,
                  .origin = scene::anchor::kCentreLeft,
                  .x = 14.0f});
    fNow.apply({.anchor = scene::anchor::kCentreRight,
                .origin = scene::anchor::kCentreRight,
                .x = -14.0f});
  }

  void again(app::requests *sink, const paints &look, float top,
             std::string_view label, std::string_view now,
             app::request asking) {
    this->apply({.anchor = scene::anchor::kTopLeft,
                 .x = 0.0f,
                 .y = top,
                 .relativeSize = scene::axes::kNone,
                 .width = editor_menu_width,
                 .height = editor_row_height,
                 .visible = true});
    fPlate.again(sink, std::move(asking), look.drawer);
    fLabel.setText(std::string(label));
    fLabel.setColour(look.text);
    // What it stands at, read where a row's own words end and not under
    // them: the same word every row keeps to its own right.
    fNow.setText(std::string(now));
    fNow.setColour(look.primary);
  }

  void hide() { this->apply({.visible = false}); }

  void forEachChild(auto &&f) {
    f(fPlate);
    f(fLabel);
    f(fNow);
  }
};

// What the three dots beside the card's ring hold: whether what answers in
// this chat is asked to think, and how hard where it is, and the way into
// the large settings for everything else a wire is told about choosing
// words. A card of its own over the messages, put away by one press of what
// opened it (design-doc.md, "The editor's row").
//
// Thinking and how hard are kept with everything else an answer is asked
// for (pchat.config, `asking`) and not here: what they are said of is every
// answer written from now on, in this conversation and in every other.
struct editor_menu : scene::Node {
  nodes::Box<> fCard{0u};
  std::array<editor_row, editor_rows> fRows;

  editor_menu() {
    fCard.apply({.fill = true, .cornerRadius = 16.0f});
    for (editor_row &one : fRows)
      one.hide();
    this->setVisible(false);
  }

  void hide() {
    this->setVisible(false);
    for (editor_row &one : fRows)
      one.hide();
  }

  void again(app::requests *sink, const paints &look, const scene::Spec &where,
             const asking &how) {
    this->setVisible(true);
    this->apply(where);
    fCard.setColour(look.drawer);
    // Thinking, and how hard it is asked to think said beside it: the word
    // for nothing at all where it is not asked to think in the first place.
    fRows[0].again(sink, look, edge, "Thinking",
                   how.thinking ? said_by(how.how_hard) : "off",
                   app::set_thinking{.on = !how.thinking});
    // And the same again on its own row, for saying it is one press away
    // from being another: each press asks for the next of the four.
    fRows[1].again(sink, look, edge + editor_row_height, "Effort",
                   said_by(how.how_hard),
                   app::set_effort{.how_hard = next_of(how.how_hard)});
    fRows[2].again(sink, look, edge + 2.0f * editor_row_height, "Advanced", "",
                   app::open_settings{});
  }

  void forEachChild(auto &&f) {
    f(fCard);
    for (editor_row &one : fRows)
      f(one);
  }
};

struct composer : scene::Node {
  nodes::Box<> fCard{0u};
  // What answers here, and how much of its window this chat has taken: said
  // over the field every time, not only when it is asked about.
  nodes::Text fModel{"", 12.0f, 0u, true};
  nodes::Icon fGauge{shape::info(), 0u};
  std::shared_ptr<message_sink> fSink = std::make_shared<message_sink>();
  widgets::TextArea<message_given> fField{widgets::Theme{}, "Message",
                                          message_given{fSink}};
  nodes::Icon fClip{shape::clip(), 0u};
  nodes::Icon fFace{shape::smile(), 0u};
  // Three dots after the ring: what they open every answer of this chat is
  // asked with. They are a plate under a mark and both take the press, for
  // the same reason every other mark of theirs does -- what a page says was
  // pressed in the middle of them is the mark painted last there, and a walk
  // up from the mark reaches the node that answers for the plate.
  press_box fDots{shape::menu()};
  // The round button that sends what has been written. It is a plate under
  // a mark and both of them take the press, for the same reason every other
  // mark of theirs does: what a page says was pressed in the middle of the
  // button is the mark painted last there, and a walk up from the mark
  // reaches this node, which answers for the plate. What it asks for is
  // read at the press and not when it is drawn, because what is written is
  // the field's to keep and not this node's.
  send_circle fSend{};
  // What was being written to each conversation when its reader left it, and
  // what is given back to the field when that reader comes back: nothing
  // written for one conversation is read in another, and nothing written in
  // one is lost by going to look at another. Kept beside the field itself,
  // which is where what is written already is.
  logic::drafts fDrafts{};
  // Which conversation what is in the field belongs to: said again every
  // frame, and what the field is read against to know when it changed.
  conversation_id fShown{};
  // What the three dots open, said over everything else drawn here and put
  // away again by one press of what opened it. It stands above the card and
  // not inside it, and nothing is clipped by the card's own edges: what a
  // node draws outside itself is drawn just the same.
  editor_menu fMenu{};

  composer() {
    // One line, which is what makes Enter send rather than put a line break
    // in: what is written here is said once, as a message, and what it keeps
    // of its shape is kept by the room and not by this field.
    fField.setSingleLine(true);
    fSend.field = &fField;
    fSend.sink = fSink;
    // Two rows in the card: over what is written, which model answers in this
    // chat and how full its window is; and under them the field itself.
    this->apply({.fillX = true,
                 .height = composer_height + 2.0f * edge,
                 .padding = scene::Margin::all(edge)});
    fCard.apply({.fill = true, .cornerRadius = card_radius});
    fModel.apply({.anchor = scene::anchor::kTopLeft,
                  .origin = scene::anchor::kCentreLeft,
                  .x = 18.0f,
                  .y = 22.0f});
    // The ring, said to the left of the three dots that follow it: which
    // model answers in this chat and how full its window this chat has made
    // it, and then what its answers are asked with.
    fGauge.apply({.width = 18.0f,
                  .height = 18.0f,
                  .anchor = scene::anchor::kTopRight,
                  .origin = scene::anchor::kCentre,
                  .x = -(edge + dots_room + 18.0f),
                  .y = 22.0f});
    fDots.apply({.width = dots_size,
                 .height = dots_size,
                 .anchor = scene::anchor::kTopRight,
                 .origin = scene::anchor::kCentre,
                 .x = -(edge + dots_size / 2.0f),
                 .y = 22.0f});
    fField.apply(
        {.anchor = scene::anchor::kTopLeft,
         .x = 16.0f,
         .y = 44.0f,
         .height = 48.0f,
         .fillX = true,
         .margin = scene::Margin{0.0f, edge + send_size + 62.0f, 0.0f, 16.0f},
         .cornerRadius = 16.0f,
         .padding = scene::Margin::horizontal(16.0f)});
    fClip.apply({.width = 22.0f,
                 .height = 22.0f,
                 .anchor = scene::anchor::kTopRight,
                 .origin = scene::anchor::kCentre,
                 .x = -(edge + send_size + 44.0f),
                 .y = 68.0f});
    fFace.apply({.width = 22.0f,
                 .height = 22.0f,
                 .anchor = scene::anchor::kTopRight,
                 .origin = scene::anchor::kCentre,
                 .x = -(edge + send_size + 16.0f),
                 .y = 68.0f});
  }

  void again(const paints &look, const conversation &chat, const asking &how,
             app::requests *asked, bool menu_open) {
    fSink->asked = asked;
    // What was in the field belonged to the conversation that was open when
    // it was written, and is kept to it before another one is opened; what
    // belongs to the one being opened comes back into the field. Nothing is
    // said of this to the program -- whose field it is and what is written
    // in it have never been anything but this card's own -- and nothing in
    // it survives the window's going, which nothing in the field did either.
    if (fShown.value != chat.id.value) {
      if (!fShown.value.empty())
        fDrafts.write(fShown, std::string(fField.plainText()));
      fField.setText(std::string(fDrafts.text_of(chat.id)));
    }
    fShown = chat.id;
    fSink->chat = chat.id;
    fCard.setColour(look.card);
    fClip.setColour(look.faint);
    fFace.setColour(look.faint);
    // What every answer in this chat is asked with, behind three dots beside
    // the ring: opened by a press on them and put away by another.
    fDots.again(asked,
                {.width = dots_size,
                 .height = dots_size,
                 .anchor = scene::anchor::kTopRight,
                 .origin = scene::anchor::kCentre,
                 .x = -(edge + dots_size / 2.0f),
                 .y = 22.0f},
                app::toggle_editor_menu{}, look.raised);
    fDots.mark_colour(look.text);
    // Lit only when there is something written to send: what it is given to
    // say is nothing at all otherwise, and a press on it asks for nothing.
    const bool written = !fField.plainText().empty();
    fSend.again({.width = send_size,
                 .height = send_size,
                 .anchor = scene::anchor::kTopRight,
                 .origin = scene::anchor::kTopRight,
                 .x = -edge,
                 .y = 45.0f,
                 .visible = true},
                written ? look.primary : look.raised,
                written ? look.on_primary : look.faint);
    // Nothing of what has been sent is kept in the field: whichever way it
    // was sent from, it is empty again before this frame is drawn.
    if (fSink->sent) {
      fSink->sent = false;
      this->clear();
    }
    // Which model, and the circle read by how much of it is drawn: its whole
    // ring said in the outline's colour, and as much of it as this chat has
    // taken said in the primary one, from the top and clockwise.
    const float used = std::clamp(chat.context_used, 0.0f, 1.0f);
    const std::string model = chat.leading_model();
    fModel.setText(said_as_a_model(model));
    fModel.setColour(look.primary);
    fGauge.setShape(context_ring(used, look.outline, look.primary));
    fGauge.setColour(look.primary);
    // What the three dots opened, over the messages and above the card.
    if (menu_open) {
      const float high = static_cast<float>(editor_rows) * editor_row_height +
                         2.0f * edge;
      fMenu.again(asked, look,
                  {.anchor = scene::anchor::kTopRight,
                   .origin = scene::anchor::kTopRight,
                   .x = -edge,
                   .y = -(high + 8.0f),
                   .relativeSize = scene::axes::kNone,
                   .width = editor_menu_width,
                   .height = high,
                   .cornerRadius = 16.0f,
                   .visible = true},
                  how);
    } else
      fMenu.hide();
  }

  // Nothing written keeps a message it has given away, and neither does what
  // was kept of it (pchat.logic.drafts, `clear`): sent is not left behind
  // to be given again by coming back to this conversation later.
  void clear() {
    fField.setText("");
    fDrafts.clear(fShown);
  }

  void forEachChild(auto &&f) {
    f(fCard);
    f(fModel);
    f(fGauge);
    f(fField);
    f(fClip);
    f(fFace);
    f(fDots);
    f(fSend);
    f(fMenu);
  }
};

} // namespace pchat::ui
