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
  // The round button that sends what has been written. It is a plate under
  // a mark and both of them take the press, for the same reason every other
  // mark of theirs does: what a page says was pressed in the middle of the
  // button is the mark painted last there, and a walk up from the mark
  // reaches this node, which answers for the plate. What it asks for is
  // read at the press and not when it is drawn, because what is written is
  // the field's to keep and not this node's.
  send_circle fSend{};

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
    fGauge.apply({.width = 18.0f,
                  .height = 18.0f,
                  .anchor = scene::anchor::kTopRight,
                  .origin = scene::anchor::kCentre,
                  .x = -(edge + 18.0f),
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

  void again(const paints &look, const conversation &chat,
             app::requests *asked) {
    fSink->asked = asked;
    fSink->chat = chat.id;
    fCard.setColour(look.card);
    fClip.setColour(look.faint);
    fFace.setColour(look.faint);
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
  }

  void clear() { fField.setText(""); }

  void forEachChild(auto &&f) {
    f(fCard);
    f(fModel);
    f(fGauge);
    f(fField);
    f(fClip);
    f(fFace);
    f(fSend);
  }
};

} // namespace pchat::ui
