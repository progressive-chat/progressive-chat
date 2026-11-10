// SPDX-License-Identifier: GPL-3.0-only
// pchat.ui:drawer -- The list of conversations down the side: what the panel
// is called, the field it searches with, Tasks and beside it Remote, a new
// chat, then one row per conversation -- its name, what was said last, how
// long ago where that is asked for, and a badge for what is unread -- and the
// settings at the foot of the list.
//
// A row is a node of its own, made with the drawer and filled in again
// whenever the model moves: the tree's shape is fixed, what it says is not.
//
// The field above the rows searches: what it is given is asked for as a
// request, the program keeps it, and the rows shown here are the ones that
// ask kept -- the search itself is pchat.logic.lists' to answer.
export module pchat.ui:drawer;

import std;
import skiff.scene;
import skiff.nodes;
import skiff.widgets.textarea;
import pchat.core;
import pchat.config;
import pchat.logic.lists;
import pchat.logic.search;
import pchat.app.requests;
import :theme;
import :base;
import :icons;

export namespace pchat::ui {

namespace scene = skiff::scene;
namespace nodes = skiff::nodes;
namespace widgets = skiff::widgets;

// What the field is told through: the field is made once, so what it asks for
// goes by this -- which the drawer keeps -- and not by anything of its own.
struct search_sink {
  app::requests *asked = nullptr;
};
// What the field gives back at every edit of it: the search asked for, as a
// request, so that the rows are what is being typed and not what Enter last
// said of it.
struct search_given {
  std::shared_ptr<search_sink> sink;
  // Nothing to give back to the scene: what is being typed is asked of the
  // program, and the press itself asks for nothing more.
  using Answer = void;
  static constexpr bool kEveryEdit = true;
  Answer operator()(std::string_view text) const {
    if (sink && sink->asked)
      sink->asked->ask(app::set_query{std::string(text)});
  }
};

// One row of the panel's own: what it is called, with its mark before the
// name, on a ground of its own and rounded as it is told to be. Pressed, it
// asks for the one thing it stands there for.
struct ask_row : scene::Node {
  nodes::Icon fMark{shape::menu(), 0u};
  nodes::Text fName{"", drawer_row_size, 0u, true};
  app::requests *asked = nullptr;
  app::request one{app::toggle_drawer{}};

  ask_row(nodes::IconShape mark, std::string name, app::request asking,
          const scene::Spec &made, bool show_mark = true)
      : one(std::move(asking)) {
    fMark.setShape(std::move(mark));
    fMark.apply({.width = drawer_mark, .height = drawer_mark,
                 .anchor = scene::anchor::kCentreLeft,
                 .origin = scene::anchor::kCentre,
                 .x = drawer_margin,
                 .visible = show_mark});
    fName.setElided(true);
    fName.setText(std::move(name));
    fName.apply({.anchor = scene::anchor::kCentreLeft,
                 .origin = scene::anchor::kCentreLeft,
                 .x = show_mark ? drawer_margin + drawer_mark + gap
                                : drawer_margin});
    this->apply(made);
  }

  [[nodiscard]] bool acceptsInput() const { return true; }
  [[nodiscard]] bool hoverChangesAppearance() const { return true; }
  [[nodiscard]] bool onClick(float, float) {
    if (asked)
      asked->ask(one);
    return true;
  }

  void again(app::requests *sink, const paints &look) {
    asked = sink;
    // The row's ground, said of the row itself and not of a plate inside it:
    // a plate fills by its own corner radius alone, and these rows are
    // rounded each corner its own -- Tasks above and Remote below, the two
    // reading as one row cut in two. Declared of the node, the corners are
    // what its ground is painted in and what its children are clipped by.
    this->apply({.background = look.field});
    fMark.setColour(look.text);
    fName.setColour(look.text);
    // One line, cut where the row ends: a name that wrapped stood its second
    // line over whatever stood under it.
    fName.apply({.maxWidth = std::max(
                     0.0f, this->bounds().width() - 2.0f * drawer_margin -
                               drawer_mark - gap)});
  }

  void forEachChild(auto &&f) {
    f(fMark);
    f(fName);
  }
};

// One conversation, as a row: the name, and under it what was said last; the
// time where it is asked for, and a badge for what is unread. Pressed, it
// opens what it names. Filled in again whenever what it says changes.
struct conversation_row : scene::Node {
  app::requests *asked = nullptr;
  conversation_id label_of;
  nodes::Box<> fAvatar{0u};
  nodes::Text fTitle{"", 15.0f, 0u, true};
  nodes::Text fWhen{"", 12.0f, 0u};
  nodes::Text fSaid{"", 13.0f, 0u};
  nodes::Box<> fBadge{0u};
  nodes::Text fCount{"", 11.0f, 0u, true};

  conversation_row() {
    this->apply({.fillX = true, .height = row_height, .cornerRadius = 16.0f});
    // One line each, cut where the row ends: a name that wraps stood its
    // second line over what was said last, and what was said last over the
    // row after it.
    fTitle.setElided(true);
    fSaid.setElided(true);
    fAvatar.apply({.width = avatar,
                   .height = avatar,
                   .cornerRadius = avatar / 2.0f,
                   .anchor = scene::anchor::kTopLeft,
                   .x = edge,
                   .y = (row_height - avatar) / 2.0f});
    fTitle.apply({.anchor = scene::anchor::kTopLeft, .y = 14.0f});
    // What a row keeps clear at its right is more than what it keeps at its
    // left: the time and the badge are read at the end of a line and not
    // against the panel's own edge.
    fWhen.apply({.anchor = scene::anchor::kTopRight,
                 .origin = scene::anchor::kTopRight,
                 .x = -drawer_right_margin * edge,
                 .y = 16.0f});
    fSaid.apply({.anchor = scene::anchor::kTopLeft, .y = 38.0f});
    fBadge.apply({.width = 20.0f,
                  .height = 20.0f,
                  .cornerRadius = 10.0f,
                  .anchor = scene::anchor::kTopRight,
                  .origin = scene::anchor::kTopRight,
                  .x = -drawer_right_margin * edge,
                  .y = 38.0f});
    fCount.apply({.anchor = scene::anchor::kTopRight,
                  .origin = scene::anchor::kCentre,
                  .x = -drawer_right_margin * edge - 10.0f,
                  .y = 48.0f});
  }

  // A row is pressed to open what it names: said as a request, and answered
  // between frames, like everything else a control asks for.
  [[nodiscard]] bool acceptsInput() const { return true; }
  [[nodiscard]] bool hoverChangesAppearance() const { return true; }
  [[nodiscard]] bool onClick(float, float) {
    if (asked)
      asked->ask(app::open_conversation{label_of});
    return true;
  }

  // Nothing of a conversation: the row is put away until one needs it.
  void blank() {
    this->setVisible(false);
    fTitle.setText("");
    fSaid.setText("");
    fWhen.setText("");
    fCount.setText("");
    fBadge.setVisible(false);
  }

  void again(app::requests *sink, const paints &look, const looks &shown,
             const conversation &chat, std::string_view said,
             std::string_view when, bool chosen) {
    asked = sink;
    label_of = chat.id;
    this->setVisible(true);
    // Which row is open said by a ground under it, declared of the row and
    // painted in its own rounded box: nothing else in the panel has one, and
    // none of the rows that are not open is painted over the panel at all.
    this->apply({.background = chosen ? look.raised : 0u});
    // A circle for its name, where one is asked for at all, and everything
    // named in the row standing over by it where there is one.
    fAvatar.setVisible(shown.avatars);
    fAvatar.setColour(look.avatar);
    const float begins = shown.avatars ? edge + avatar + gap : edge;
    fTitle.apply({.x = begins});
    fSaid.apply({.x = begins});
    fTitle.setText(chat.title);
    fTitle.setColour(look.text);
    fSaid.setText(std::string(said));
    fSaid.setColour(look.dim);
    // How long ago, where it is asked for: as the clock reads it, and the
    // row is read without it where it is not.
    fWhen.setVisible(shown.row_times);
    fWhen.setText(std::string(when));
    fWhen.setColour(look.faint);
    fBadge.setVisible(chat.unread > 0);
    fBadge.setColour(look.badge);
    fCount.setVisible(chat.unread > 0);
    fCount.setText(chat.unread > 99 ? "99+" : std::to_string(chat.unread));
    fCount.setColour(look.badge_text);
  }

  void forEachChild(auto &&f) {
    f(fAvatar);
    f(fTitle);
    f(fWhen);
    f(fSaid);
    f(fBadge);
    f(fCount);
  }
};

// The drawer: its own colour behind everything, its heading, the field it
// searches with, the two rows beside each other, the new chat, the rows, and
// the settings at the foot of them.
struct drawer_screen : scene::Node {
  // Said while the rows are being looked into: the room a row was given, the
  // reach its texts were given of it, and where the first row's own name and
  // last words came to stand -- whether one stands over the other.

  nodes::Box<> fPaper{0u};
  nodes::Text fHeading{"Conversations", drawer_heading_size, 0u, true};
  // Said where the search kept nothing at all: the drawer standing empty
  // says less than a line saying why.
  nodes::Text fNone{"No conversation is named that, or says it.", 13.0f, 0u};
  // The magnifier, in the field's own ground and before what it says: the
  // field is a control of its own kind, and nothing else is drawn in it.
  nodes::Icon fSearchMark{shape::search(), 0u};
  std::shared_ptr<search_sink> fSink = std::make_shared<search_sink>();
  widgets::TextArea<search_given> fSearch{
      widgets::Theme{}, "Search conversations", search_given{fSink}};
  // Two rows on one ground, rounded between them: pressed, the one says what
  // runs here on its own, and the other what another machine is spoken to
  // through. Neither stands behind anything in the program yet.
  ask_row fTasks{shape::done(), "Tasks", app::open_tasks{},
                 {.anchor = scene::anchor::kTopLeft,
                  .x = edge,
                  .fillX = true,
                  .height = drawer_row_height,
                  .corners = scene::Corners{.topLeft = drawer_row_radius,
                                            .topRight = drawer_row_radius,
                                            .bottomRight = drawer_row_corner,
                                            .bottomLeft = drawer_row_corner}}};
  ask_row fRemote{shape::system_prompt(), "Remote", app::open_remote{},
                  {.anchor = scene::anchor::kTopLeft,
                   .x = edge,
                   .fillX = true,
                   .height = drawer_row_height,
                   .corners = scene::Corners{.topLeft = drawer_row_corner,
                                             .topRight = drawer_row_corner,
                                             .bottomRight = drawer_row_radius,
                                             .bottomLeft = drawer_row_radius}}};
  // A new chat, and the settings: the one rounded the whole way round, and
  // the other at the foot of the list, which is the panel's only way into
  // the page over the room.
  ask_row fNewChat{shape::plus(), "+ New Chat", app::new_chat{},
                   {.anchor = scene::anchor::kTopLeft,
                    .x = edge,
                    .fillX = true,
                    .height = drawer_row_height,
                    .cornerRadius = drawer_row_height / 2.0f},
                   false};
  ask_row fSettings{shape::gear(), "Settings", app::open_settings{},
                    {.anchor = scene::anchor::kTopLeft,
                     .x = edge,
                     .fillX = true,
                     .height = drawer_row_height,
                     .cornerRadius = drawer_row_height / 2.0f}};
  // One row for every conversation the search kept, made as they are asked
  // for and kept once made: nothing here is said two ways for being asked
  // for twice, and no conversation is left out for there being many.
  std::vector<std::unique_ptr<conversation_row>> fRows;
  // Where the rows begin: under the new chat, said once here because the
  // rows are put out from it and the settings stand at the foot of them.
  float rows_top = 0.0f;

  // The row for this place down the drawer, made there where there is none:
  // every row says where it stands, told the place it was asked for.
  [[nodiscard]] conversation_row &row_at(std::size_t place, float text) {
    while (fRows.size() <= place)
      fRows.push_back(std::make_unique<conversation_row>());
    conversation_row &row = *fRows[place];
    row.apply({.anchor = scene::anchor::kTopLeft,
               .x = edge,
               .y = rows_top + static_cast<float>(place) * (row_height + 4.0f)});
    // How far the name and what was said reach: the room the row was given,
    // less the circle's and less what the time and the badge keep at its end
    // -- a width of their own was one written for one width of drawer, and
    // left them ended half way over a wider one.
    row.fTitle.apply({.maxWidth = text});
    row.fSaid.apply({.maxWidth = text});
    return row;
  }

  explicit drawer_screen(float breadth) {
    this->apply({.width = breadth, .fillY = true});
    fPaper.apply({.fill = true});
    // The field, its magnifier standing in it before what it says: one line,
    // and its own ground and hint said again whenever the colours are.
    fSearch.setSingleLine(true);
    fHeading.apply({.anchor = scene::anchor::kTopLeft,
                    .x = drawer_margin,
                    .y = drawer_margin});
    // The field, its magnifier standing in it before what it says.
    const float search_y = drawer_margin + 34.0f;
    fSearch.apply({.anchor = scene::anchor::kTopLeft,
                   .x = edge,
                   .y = search_y,
                   .fillX = true,
                   .height = 40.0f,
                   .cornerRadius = 20.0f,
                   .padding = scene::SpecMargin(
                       0.0f, drawer_margin, 0.0f,
                       drawer_margin + drawer_mark + gap)});
    fSearchMark.apply({.anchor = scene::anchor::kTopLeft,
                       .origin = scene::anchor::kCentre,
                       .width = drawer_mark,
                       .height = drawer_mark,
                       .x = edge + drawer_margin,
                       .y = search_y + 20.0f});
    // Tasks and Remote: one ground in two rows, only a few pixels apart, and
    // the one a new chat begins from under them.
    const float tasks_y = search_y + 40.0f + gap;
    const float remote_y = tasks_y + drawer_row_height + drawer_row_gap;
    const float new_y = remote_y + drawer_row_height + gap;
    rows_top = new_y + drawer_row_height + gap;
    fTasks.apply({.anchor = scene::anchor::kTopLeft, .x = edge, .y = tasks_y});
    fRemote.apply(
        {.anchor = scene::anchor::kTopLeft, .x = edge, .y = remote_y});
    fNewChat.apply({.anchor = scene::anchor::kTopLeft, .x = edge, .y = new_y});
    fNone.apply({.anchor = scene::anchor::kTopLeft,
                 .x = drawer_margin,
                 .y = rows_top});
    fNone.setVisible(false);
  }

  // The colours, then what each row says: called by the program whenever the
  // model moved. What the rows are is what the search asked for keeps -- the
  // search is not this drawer's to do; the drawer only shows what was kept.
  void again(const paints &look, const looks &shown, const chats_root &root,
             const std::string &opened, const std::string &query,
             app::requests *asked) {
    fPaper.setColour(look.drawer);
    fHeading.setColour(look.text);
    fNone.setColour(look.faint);
    fSearchMark.setColour(look.faint);
    // The field made of the same colours every other field is: its ground,
    // the hint grey before anything has been typed into it.
    widgets::Theme theme{};
    theme.fSurface = look.field;
    theme.fText = look.text;
    theme.fTextDim = look.dim;
    theme.fTextFaint = look.faint;
    theme.fAccent = look.primary;
    fSearch.setTheme(theme);
    // A TextArea draws words and a caret and nothing else: no ground of its
    // own, so the field there is searched with was no field at all -- only a
    // hint standing on the panel. Its ground is said of the node, rounded
    // the whole way round as the field is asked to be.
    fSearch.apply({.background = look.field});
    for (ask_row *one : {&fTasks, &fRemote, &fNewChat, &fSettings})
      one->again(asked, look);
    fSink->asked = asked;
    const float row_room = this->bounds().width() - 2.0f * edge;
    const float text = std::max(
        60.0f, row_room - edge - drawer_right_margin * edge -
                   (shown.avatars ? avatar + gap : 0.0f) -
                   (shown.row_times ? 64.0f : 24.0f));
    const auto rows = logic::matching(root, query);
    fNone.setVisible(rows.empty());
    for (std::size_t at = 0; at < rows.size(); ++at) {
      const conversation &chat = *rows[at];
      // What the row says under the name: where something was searched for
      // and this conversation says it, the message it is said in, which is
      // what the row was kept for; else the last message in it, if there is
      // one, else what the conversation is.
      std::string said = chat.kind == conversation_kind::direct
                              ? "Say something"
                              : "No messages yet";
      for (const message &one : root.messages)
        if (one.chat.value == chat.id.value)
          said = one.text;
      const std::vector<logic::hit> places =
          logic::hits_of(root, chat.id, query);
      if (!places.empty())
        said = places.front().found->text;
      // How long ago, as the clock reads it: the moment the last thing was
      // said in it, which is what the order of these rows comes from too.
      this->row_at(at, text).again(asked, look, shown, chat, said,
                                   clock_of(chat.last_activity),
                                   chat.id.value == opened);
    }
    // At the foot of them, whatever the list came to, the row the settings
    // are opened from.
    fSettings.apply({.anchor = scene::anchor::kTopLeft,
                     .x = edge,
                     .y = rows_top + static_cast<float>(rows.size()) *
                                         (row_height + 4.0f)});
    // What is left of the list where fewer conversations were kept than were
    // kept here before: said by nobody.
    for (std::size_t at = rows.size(); at < fRows.size(); ++at)
      fRows[at]->blank();
  }

  void forEachChild(auto &&f) {
    f(fPaper);
    f(fHeading);
    f(fNone);
    f(fSearchMark);
    f(fSearch);
    f(fTasks);
    f(fRemote);
    f(fNewChat);
    for (auto &row : fRows)
      f(*row);
    f(fSettings);
  }
};

} // namespace pchat::ui
