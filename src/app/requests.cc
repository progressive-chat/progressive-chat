// SPDX-License-Identifier: AGPL-3.0-only
// pchat.app.requests -- What a screen asks the program for, and nothing else
// about the program: a list the screen pushes into and the program drains
// between frames.
//
// A control's handler is running while it is handling. It does not switch the
// screen or rebuild a list from inside itself -- it says what it wants, and
// the program does it before the next frame.
export module pchat.app.requests;

import splice;
import std;
import pchat.core;
import pchat.config;
import pchat.theme;

export namespace pchat::app {

struct open_conversation {
  conversation_id chat;
};
struct send_message {
  conversation_id chat;
  std::string text;
};
// A new chat, asked for at the top of the messages: nothing in it but the
// one that asked.
struct new_chat {};
// What one answer is asked for, down at its foot: taken whole onto the
// clipboard, thought again, taken into a chat by itself, given away,
// asked about, or rubbed out.
struct copy_answer {
  conversation_id chat;
  message_id said;
};
struct regenerate_answer {
  conversation_id chat;
  message_id said;
};
struct fork_answer {
  conversation_id chat;
  message_id said;
};
struct share_answer {
  conversation_id chat;
  message_id said;
};
struct delete_answer {
  conversation_id chat;
  message_id said;
};
struct set_query {
  std::string text;
};
// What one line of the page gives away: pressed, what it says is given to
// the machine's own clipboard, whole.
struct copy_text {
  std::string text;
};
struct toggle_drawer {};
struct open_settings {};
struct close_settings {};
// The two rows the drawer keeps beside its search: what the program has been
// told to do on its own, and what another machine is spoken to through.
// Neither stands behind either row yet; asking for one is answered by
// nothing until what it names is.
struct open_tasks {};
struct open_remote {};
// What the top edge asks for: another answerer in this same conversation,
// this conversation taken whole into one by itself, this conversation given
// out, and what every answerer in it is told before anything is said to it.
struct fork_conversation {
  conversation_id chat;
};
struct share_conversation {
  conversation_id chat;
};
// Three dots, and what stands behind them: one press to open what they hold
// and one to put it away again.
// What every answerer in a conversation is told: opened to be read, written
// into the conversation as it is typed, and put away again.
struct open_system_prompt {
  conversation_id chat;
};
struct close_system_prompt {};
struct set_system_prompt {
  conversation_id chat;
  std::string text;
};
struct toggle_more {};
// The same three dots again, where they stand at the card's own foot: one
// press opens what they hold and one puts it away.
struct toggle_editor_menu {};
// The face beside the clip: what it opens is the card of faces, and one of
// them pressed writes itself into the field (src/ui/composer.cc), which is
// why nothing the program keeps is asked for it.
struct toggle_faces {};
// One face pressed, sent to the program so it is one of the ones pressed
// most lately and drawn first in the card: what the press writes into the
// field is still not asked of it -- the field is the card's own -- but which
// ones were pressed, and in what order, the program keeps, for this run and
// the runs after.
struct face_written { std::string face; };
// What a row behind them says (design-doc.md, "The editor's row"): whether
// what answers here is asked to think at all before it answers, and how hard
// it is asked to think where it is. Neither is a row of the settings, for
// both are said of this conversation's own card and not of the program.
struct set_thinking {
  bool on = false;
};
struct set_effort {
  effort how_hard = effort::some;
};
// The design said in the room itself: what stands written at the top of the
// tree is read here too, and put away again.
struct open_design {};
struct close_design {};
struct set_amoled {
  bool enabled;
};
// What a row in the drawer is asked for, from the Appearance page: whether a
// circle stands for the conversation it names, and whether it says how long
// ago the last thing in it was said. Neither is drawn until it is asked for
// here -- a panel read by what its conversations are called.
struct set_avatars {
  bool enabled;
};
struct set_row_times {
  bool enabled;
};
// What the Notifications page asks for: one test notification, shown by the
// machine it runs on, so it can be seen whether it is let show any. Nothing
// it pretends is a message.
struct send_test_notification {};
// What the Appearance page asks for: which seed the colours grow out of,
// whether the type is set larger, and whose face it is set in. Each row asks
// for one whole thing, not for the set of looks it would like to see, because
// what the rest of the looks are is the program's to know and not a row's.
struct pick_seed {
  scheme_preset preset = default_preset;
};
struct set_font_scale {
  double scale = 1.0;
};
// Which way the colours go: light or dark, whichever the seed is grown in.
struct pick_theme {
  theme_mode mode = theme_mode::dark;
};
// How an answer is to be asked for (design-doc.md, "Generation"): one of the
// five things a wire is told about choosing words, and what it is set to.
// A row says the one it was pressed for and nothing of the others, the same
// way a row of the Appearance page says its own switch alone.
struct set_generation {
  asking_what field = asking_what::temperature;
  double value = 0.0;
};
// One of the tools this program owns, switched: on, and a model is told it
// may ask for it; off, and no answer this program writes mentions it.
struct set_tool {
  tool_choice which = tool_choice::conversation_search;
  bool on = false;
};
// One of the seven a switch is offered for, switched: whether it is
// offered at all, which a row of the settings page says by which way it
// stands (pchat.config, `switch_choice`).
struct set_switch {
  switch_choice which = switch_choice::picture_to_words;
  bool on = false;
};
// One of the answerers offered (pchat.config), in a conversation or out of
// it: which of them answers where nothing else has been said.
struct set_answerer {
  std::string name;
  bool wanted = false;
};
// The archive of everything this program holds, written into one .prgrssv file
// where the machine keeps files: asked for on the settings page's About, and
// answered with a save dialog of the machine's own -- the chosen path comes
// back through the program (pchat.app.program, save_path_chosen).
struct save_archive {};

// What is asked, as one value: spl::variant, so every kind of it is handled
// where it is read and a new one cannot be forgotten.
using request = spl::variant<open_conversation, send_message, new_chat,
                             copy_answer, regenerate_answer, fork_answer,
                             share_answer, delete_answer, set_query,
                             toggle_drawer, open_settings, close_settings,
                             open_tasks, open_remote,
                             fork_conversation,
                             share_conversation, open_system_prompt,
                             close_system_prompt, set_system_prompt,
                             toggle_more, open_design, close_design,
                             toggle_editor_menu, toggle_faces, face_written,
                             set_thinking,
                             set_effort,
                             pick_seed, set_amoled, set_font_scale, pick_theme,
                                set_avatars, set_row_times,
                                send_test_notification,
                                set_generation, set_tool, set_switch,
                              set_answerer, copy_text, save_archive>;

class requests {
public:
  template <class asking> void ask(asking &&one) {
    pending_.emplace_back(std::forward<asking>(one));
  }

  // What has been asked since the last frame: drained whole, as the program
  // reads each one in the order it was asked for.
  [[nodiscard]] std::vector<request> drain() { return std::move(pending_); }

  [[nodiscard]] bool empty() const { return pending_.empty(); }

private:
  std::vector<request> pending_;
};
// A kind of request is read with `spl::visit(spl::overloaded{...}, one)`:
// one callable per kind, as splice gives it.
} // namespace pchat::app
