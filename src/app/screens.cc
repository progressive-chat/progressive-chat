// SPDX-License-Identifier: AGPL-3.0-only
// pchat.app.screens -- What the window shows, and what a control's asking is
// answered with: one model, one set of looks, one scene. Every request a
// control asked for is answered here, between frames, and the tree is filled
// in again from the model before the frame is drawn.
//
// Nothing here talks to a server: what fills the model is what the last run
// kept, where this machine keeps such things, and the sample until there is
// one -- and after that, until the client is.
module;
#include "../platform/state.h"

export module pchat.app.screens;

import splice;
import std;
import skiff.scene;
import skiff.nodes;
import skiff.platform.dialogs;
import skiff.platform.storage;
import pchat.core;
import pchat.config;
import pchat.theme;
import pchat.ui;
import pchat.platform.notified;
import pchat.platform.opened;
import pchat.provider;
import pchat.provider.budget;
import pchat.provider.run;
import pchat.platform.carry;
import pchat.provider.loop;
import pchat.tools;
import pchat.prgrssv;
import pchat.app.requests;
import pchat.app.sample;
import pchat.logic.composer;
import pchat.logic.lists;

namespace scene = skiff::scene;

export namespace pchat::app {

// The tree the host draws, and what a Scene holds it as: said here, so that
// the program needs nothing of the screens but these.
using window_scene = scene::Scene<ui::window_type>;

// Everything shown, and what is done with what was asked for.
class screens {
public:
  screens()
      : root_(sample_chats()), shown_(default_looks),
        look_(ui::paints_of(colors_of(shown_, true), true)) {
    // What the last run kept, read before the first frame is drawn: whole, in
    // place of the sample, where this machine keeps such things -- the sample
    // is what stands until there is something real to stand. What could not be
    // read is what was not kept, and the sample stands there all the same.
    if (platform::keeps_state()) {
      const std::string kept = platform::state_read();
      if (!kept.empty()) {
        prgrssv::restored back = prgrssv::restore_of(kept);
        if (back.ok()) {
          root_ = std::move(back.root);
          shown_ = back.shown;
          how_ = back.how;
          where_ = std::move(back.where);
          look_ = ui::paints_of(colors_of(shown_, true), true);
        }
      }
    }
  }

  // What this run held, kept for the run after it: the whole archive, said
  // where this machine keeps such things, and nowhere at all where it does
  // not. A run whose state could not be written is still the run it was.
  ~screens() {
    if (platform::keeps_state())
      platform::state_write(prgrssv::archive_of(root_, shown_, how_, where_));
  }

  [[nodiscard]] window_scene &window() noexcept { return scene_; }

  // How answers are asked for, as it stands now, and as one row of the
  // settings may move it: one of the five, kept inside its own lines.
  [[nodiscard]] const asking &how() const noexcept { return how_; }
  void how(asking_what field, double value) { how_ = set_of(how_, field, value); }

  // Answered between frames: what the controls asked for, oldest first.
  void take(const request &one) {
    spl::visit(
        spl::overloaded{
            [&](const open_conversation &asking) {
              root_.opened = asking.chat;
              // The card of faces belongs to the conversation it was opened
              // in: nothing written into it follows its reader elsewhere.
              faces_ = false;
              // A drawer that came over the messages has been used:
              // put away, leaving the conversation it opened.
              if (width_ < ui::side_by_side_from)
                drawer_ = false;
            },
            [&](const send_message &asking) {
              const logic::sent_message said = logic::send(
                  logic::sending_request{asking.chat, asking.text,
                                         asking.attachments, std::nullopt},
                  "Reader", static_cast<moment>(said_count_++));
              root_.messages.push_back(said.said);
              // What was attached went with the message whole -- the card is
              // clear of it again before this frame is drawn, the same way
              // the field is (the composer clears itself on `sent`).
              attached_.clear();
              recount(asking.chat);
              // At the edge of the window again: what would be cut from the
              // wire to fit is older context nobody would ever see again, so
              // it is summarised into a capsule first -- as Agora starts an
              // automatic compact before a send would overflow (90% of the
              // budget, its own default) -- and the question itself goes out
              // when that lands. Below the edge, asked at once, as always;
              // and where the compact could not even begin, the question
              // goes out at once too.
              // And where the reader asked for this to happen at all: the
              // switch is what says so (Generation page), and off is as
              // automatic as this program has ever been -- fit the window
              // the way it always fit.
              if (offered_of(how_, switch_choice::auto_compact) &&
                  overruns_of(asking.chat)) {
                pending_ask_ = true;
                begin_ask(asking.chat, std::string(compact_told), true);
              }
              if (!answering_) {
                pending_ask_ = false;
                ask_model(asking.chat);
              }
              // What was written has left the field, and the card that
              // writes into it goes with it: nothing left open would write
              // into a field that has already been given away.
              faces_ = false;
            },

            [&](const new_chat &) {
              const conversation_id made{"chat-" +
                                         std::to_string(++chat_count_)};
              conversation fresh{
                  made,
                  root_.accounts.empty() ? account_id{}
                                         : root_.accounts.front().id,
                  std::string(new_chat_title), conversation_kind::direct,
                  ++said_count_};
              if (root_.opened)
                fresh.models.push_back(model_of(*root_.opened));
              // The room every answerer in it has, said by the first of them:
              // a chat made new has all of its window to talk into.
              fresh.window = window_of(fresh.leading_model());
              root_.conversations.insert(root_.conversations.begin(),
                                         std::move(fresh));
              root_.opened = made;
              if (width_ < ui::side_by_side_from)
                drawer_ = false;
            },
            // Taken whole, so that it can be put down somewhere else.
            [&](const copy_answer &asking) { this->give(text_of(asking.said)); },
            // One line of the settings page: what it says is given whole.
            [&](const copy_text &asking) { this->give(asking.text); },
            // Asked again: this answer taken away, and every one after it, so
            // that the question above it is open to be answered again -- and
            // asked again it is, then and there, for a question left open is
            // not what this one was pressed for. Before, it only took the
            // answer away: pressed again, again, a conversation lost an
            // answer each time and never had one back, though what asks for
            // one was already here and only this line kept it out.
            [&](const regenerate_answer &asking) {
              cut_away(asking.said);
              ask_model(asking.chat);
            },
            // Into a chat of its own: everything asked and answered up to this
            // answer, and nothing said after it.
            [&](const fork_answer &asking) {
              const conversation_id made{"chat-" +
                                         std::to_string(++chat_count_)};
              conversation forked{
                  made,
                  root_.accounts.empty() ? account_id{}
                                         : root_.accounts.front().id,
                  "Forked chat", conversation_kind::direct, ++said_count_};
              forked.models.push_back(model_of(asking.chat));
              forked.tokens = tokens_up_to(asking.chat, asking.said);
              root_.conversations.insert(root_.conversations.begin(),
                                         std::move(forked));
              const std::size_t up_to = message_index(asking.said);
              for (std::size_t at = 0; at < up_to; ++at) {
                message copy = root_.messages[at];
                if (copy.chat != asking.chat)
                  continue;
                copy.chat = made;
                copy.id = message_id{made.value + "-" + copy.id.value};
                root_.messages.push_back(std::move(copy));
              }
              recount(made);
              root_.opened = made;
            },
            // Given away as it reads, until the program has somewhere to give
            // it to.
            [&](const share_answer &asking) { this->give(text_of(asking.said)); },
            [&](const delete_answer &asking) { cut_away(asking.said); },
            [&](const set_query &asking) { query_ = asking.text; },
            [&](const toggle_drawer &) { drawer_ = !drawer_; },
            // The settings are opened from the card's own three dots as well
            // as from the drawer, and what opened them is put away either
            // way: two cards over each other are one too many.
            [&](const open_settings &) {
              settings_ = true;
              editor_menu_ = false;
            },
            [&](const close_settings &) { settings_ = false; },
            // What the drawer's two rows are there for: nothing stands behind
            // either of them yet, and so nothing is opened. They are drawn
            // where the panel has them, and what they ask for is answered
            // the day there is something to answer it with.
            [&](const open_tasks &) {},
            [&](const open_remote &) {},
            [&](const fork_conversation &asking) {
              conversation *said = nullptr;
              for (conversation &one : root_.conversations)
                if (one.id == asking.chat)
                  said = &one;
              if (!said)
                return;
              const conversation_id made{"chat-" +
                                         std::to_string(++chat_count_)};
              conversation forked{
                  made, said->account, said->title + " (forked)",
                  said->kind, ++said_count_};
              forked.models = said->models;
              forked.tokens = said->tokens;
              forked.window = said->window;
              forked.context_used = said->context_used;
              std::vector<message> taken;
              for (message copy : root_.messages)
                if (copy.chat == asking.chat) {
                  copy.chat = made;
                  copy.id = message_id{made.value + "-" + copy.id.value};
                  taken.push_back(std::move(copy));
                }
              const conversation_id kept = root_.opened
                                              ? *root_.opened
                                              : conversation_id{};
              root_.conversations.insert(root_.conversations.begin(),
                                         std::move(forked));
              for (message &copy : taken)
                root_.messages.push_back(std::move(copy));
              root_.opened = made;
              recount(made);
              (void)kept;
            },
            // Given away as it reads, until the program has somewhere to give
            // it to: everything said in the conversation, in the order it was
            // said in.
            [&](const share_conversation &asking) {
              std::string whole;
              for (const conversation &one : root_.conversations)
                if (one.id == asking.chat)
                  whole += one.title + "\n";
              for (const message &one : root_.messages)
                if (one.chat == asking.chat)
                  whole += (one.outgoing ? "You: " : "") + one.text + "\n";
              if (!whole.empty())
                this->give(whole);
            },
            [&](const toggle_more &) { more_ = !more_; },
            // What the card's three dots hold: thinking, how hard it is asked
            // to think, and the large settings. Thinking and how hard are how
            // every answer in every conversation is asked for from now on
            // (pchat.config, `asking`), which is what the settings' own rows
            // would have said of them.
            [&](const toggle_editor_menu &) { editor_menu_ = !editor_menu_; },
            // The card of faces above the face beside the clip: opened by a
            // press on it, and put away by another press anywhere else in
            // the room, which is what nothing measured asks for
            // (src/ui/window.cc).
            [&](const toggle_faces &) { faces_ = !faces_; },
            // One face pressed: to the front of the ones pressed most
            // lately, wherever in them it stood before, and no longer a row
            // than the card draws. Kept as they were pressed and not as
            // often, so that what comes first is the one that was wanted
            // last and not the one that was wanted most often.
            [&](const face_written &asking) {
              if (asking.face.empty()) return;
              std::erase(faces_lately_, asking.face);
              faces_lately_.insert(faces_lately_.begin(), asking.face);
              if (faces_lately_.size() > ui::faces_lately_kept)
                faces_lately_.resize(ui::faces_lately_kept);
            },
            [&](const set_thinking &asking) { how_.thinking = asking.on; },
            [&](const set_effort &asking) { how_.how_hard = asking.how_hard; },
            // What every model is told before anything is said to it: read
            // and written whole, the conversation keeping it as it is typed.
            [&](const set_system_prompt &asking) {
              for (conversation &one : root_.conversations)
                if (one.id == asking.chat)
                  one.told = asking.text;
            },
            [&](const open_system_prompt &) {
              more_ = false;
              system_ = true;
            },
            [&](const close_system_prompt &) { system_ = false; },
            // Read where it is asked for, over the room and over nothing
            // else: the page it was asked from is put away under it, so
            // that putting the design away leaves the conversation and not
            // a page the reader had already gone on from.
            [&](const open_design &) {
              settings_ = false;
              design_ = true;
            },
            [&](const close_design &) { design_ = false; },
            // What the seed is: every colour grown again out of it, and the
            // page staying open to be looked at.
            [&](const pick_seed &asking) {
              shown_.preset = asking.preset;
              recolour();
            },
            [&](const set_amoled &asking) {
              shown_.amoled = asking.enabled;
              recolour();
            },
            // What the drawer's rows are then drawn with: no colour grows
            // out of either of these, and so nothing is grown again.
            [&](const set_avatars &asking) { shown_.avatars = asking.enabled; },
            [&](const set_row_times &asking) {
              shown_.row_times = asking.enabled;
            },
            // A test notification: nothing of the program's own knows whether
            // the machine it runs on will show it, so all this does is ask the
            // platform and go on.
            [&](const send_test_notification &) {
              platform::notified::say("progressive-chat",
                                      "This is a test notification.");
            },
            // Larger type: every size drawn in the body of a message is asked
            // for again with this multiply in it, so what is set larger is
            // what the answers are written in.
            [&](const set_font_scale &asking) {
              shown_.font_scale = asking.scale;
              recolour();
            },
            [&](const pick_theme &asking) {
              shown_.mode = asking.mode;
              dark_ = dark_of(shown_.mode, true);
              recolour();
            },
            // How an answer is asked for: the one of the five a row was
            // pressed for, kept inside the lines it is said to have
            // (pchat.config, `asking`).
            [&](const set_generation &asking) { how(asking.field, asking.value); },
            // A tool switched: one of the three this program owns, and what
            // it is switched to.
            [&](const set_tool &asking) {
              how_ = set_of(how_, asking.which, asking.on);
            },
            // One of the seven switched: what a row of the settings page
            // said, and nothing of the others.
            [&](const set_switch &asking) {
              how_ = set_of(how_, asking.which, asking.on);
            },
            // One of the doors chosen: which one the next asking is spoken
            // through -- kept with the settings, so that the run after this
            // one asks through the same, and by a word that stands among the
            // offered ones even where a press named something else.
            [&](const pick_door &asking) {
              where_.provider = std::string(provider_by(asking.id).id);
            },
            // The key for one of them, whole as it was typed: into the door
            // itself, where it stays with the settings, and out nowhere
            // before an asking goes through that door with it (src/provider.
            // cc, headers_of). Which door it opens stands with it -- a key
            // is not a key without its door.
            [&](const set_door_key &asking) {
              key_into(where_, asking.id, asking.key);
            },
            // What one of the doors may answer as, whole as it was written:
            // ids apart, for the door the field stands at -- kept with that
            // door, and offered in a chat's models beside the answerers
            // known by name. Typing one here changes no chat; a chat takes
            // a model by its row (set_answerer, above).
            [&](const set_door_models &asking) {
              models_into(where_, asking.id, text_to_models(asking.typed));
            },
            // The compact asked for by hand: this chat's older stretch
            // summarised into a capsule, originals where they stand. Asked
            // of a chat that has said nothing, there is nothing to hand
            // over, and nothing is asked.
            [&](const compact_context &) {
              if (!root_.opened)
                return;
              const conversation_id chat = *root_.opened;
              const bool has_messages = std::ranges::any_of(
                  root_.messages,
                  [&chat](const message &one) { return one.chat == chat; });
              if (!has_messages)
                return;
              begin_ask(chat, std::string(compact_told), true);
            },
            // The archive asked for: which files are kept where is the
            // machine's own to say, and all this says is the name the file
            // goes under -- the bytes are written when a path comes back
            // (write_archive, through the program).
            [&](const save_archive &) {
              if (dialogs_)
                dialogs_->choose_save_path("progressive-chat.prgrssv");
            },
            // The archive to read, offered by the machine's own picker of
            // files: what it answers with comes back through the program
            // (files_given), on the same road the save's path answers on --
            // and what is picked now is known to be wanted for this, since
            // one picker stands behind both this and the clip below.
            [&](const load_archive &) {
              files_wanted_ = files_want::archive;
              if (dialogs_)
                dialogs_->choose_files();
            },
            // Files to attach to what is being written: the same picker, a
            // different word for what is wanted of it (files_arrived).
            [&](const attach_files &) {
              files_wanted_ = files_want::attach;
              if (dialogs_)
                dialogs_->choose_files();
            },
            // What is attached put away whole, as one press on its names
            // below the field asks for.
            [&](const detach_files &) { attached_.clear(); },
            // An answerer put into the conversation open, or taken out of
            // it: what the window then writes to is the first one left, and
            // how much room it has to answer into is that one's own.
            [&](const set_answerer &asking) {
              conversation *open = opened_conversation();
              if (!open)
                return;
              open->models = models_with(open->models, asking.name,
                                         asking.wanted);
              open->window = window_of(open->leading_model());
              recount(open->id);
            }},
        one);
  }

  // The window's own size, as the layout of the last frame left it.
  void sized(float width, float height) noexcept {
    width_ = width;
    height_ = height;
  }

  // Everything asked for since the last frame, answered, and the tree filled
  // in again from the model.
  void before_frame(double) {
    for (const request &one : asked_.drain())
      take(one);
    // A link pressed in a message since the last frame: skiff says which
    // ones were pressed, standing in whatever text they were drawn in, and
    // each is handed to the machine to open -- a browser of its own on a
    // desk and a phone, another tab where this is a page. Nothing of what
    // opens then is this program's to know.
    for (const std::string &place : std::exchange(scene::hostWork().links, {}))
      platform::opened::open(place);
    poll_response();
    const std::vector<logic::attachment> &attached = attached_open_chat();
    scene_.root().again(look_, root_, shown_, how_, where_, query_, dark_,
                        ui::open_state{.settings = settings_,
                                         .design = design_,
                                         .prompt = system_,
                                         .more = more_,
                                         .editor_menu = editor_menu_,
                                         .faces = faces_,
                                         .drawer = drawer_},
                        faces_lately_, attached, width_, height_, &asked_);
  }

  [[nodiscard]] chats_root &root() noexcept { return root_; }
  [[nodiscard]] const chats_root &root() const noexcept { return root_; }
  [[nodiscard]] bool dark() const noexcept { return dark_; }

  // Where the machine's own file dialogs are asked for: the program holds
  // them (they answer on its thread), and this screen is the one that knows a
  // save is wanted and what the file is called. Nothing here is a dialog --
  // the chosen path comes back through the program (save_path_chosen).
  void dialogs_from(skiff::platform::dialogs::dialogs &one) {
    dialogs_ = &one;
  }

  // Where the machine said the archive stands: the bytes built whole and given
  // to the platform once -- a page turns the file written there into a
  // download under its own name, a desk leaves it where it was asked to stand
  // (skiff.platform.storage).
  void write_archive(const std::string &path) {
    skiff::platform::storage::write_file(
        path, prgrssv::archive_of(root_, shown_, how_));
  }

  // Where the machine said an archive stands to be read from: whole, and
  // what it holds replaces what this run holds -- an archive IS a program,
  // settings and all (prgrssv, restore_of), so there is nothing to merge and
  // nothing to ask: what it was written as is what the window shows from
  // here on. A file that is not one of these, or cannot be read, changes
  // nothing -- the run keeps everything it had, whole.
  void read_archive(const std::string &path) {
    const std::optional<std::string> bytes =
        skiff::platform::storage::read_file(path);
    if (!bytes)
      return;
    prgrssv::restored back = prgrssv::restore_of(*bytes);
    if (!back.ok())
      return;
    // An answer in the air over the run being replaced is dropped with it:
    // what it was answering is not here anymore, and half an answer written
    // into a conversation this file does not hold is no answer to anything.
    answering_.reset();
    root_ = std::move(back.root);
    shown_ = back.shown;
    how_ = back.how;
    where_ = std::move(back.where);
    recolour();
  }

  // What its own picker offered, said where it was asked for: an archive to
  // read whole where the page asked for one (load_archive), files to attach
  // where the card asked for them (attach_files) -- and anything dropped
  // over the window attached too, which is what a reader means by dropping
  // a file. What is not text to read is not attached at all: a picture or an
  // archive said into a message as words would be said wrong
  // (pchat.logic.composer, attachment_text_of).
  void files_arrived(const std::vector<std::string> &paths, bool dropped) {
    const bool attaching = dropped || files_wanted_ == files_want::attach;
    files_wanted_ = files_want::nothing;
    if (paths.empty())
      return;
    if (!attaching) {
      read_archive(paths.front());
      return;
    }
    for (const std::string &one : paths) {
      const std::optional<std::string> bytes =
          skiff::platform::storage::read_file(one);
      if (!bytes)
        continue;
      std::optional<std::string> text = logic::attachment_text_of(*bytes);
      if (!text)
        continue;
      attached_.push_back(
          logic::attachment{logic::file_name_of(one), std::move(*text)});
    }
  }

  // What is attached to what the window has open: kept for that conversation
  // alone, and put away the moment another one is opened -- what is attached
  // for one conversation is not carried into another, the same as what is
  // typed into the field (whose own drafts say so of it).
  [[nodiscard]] const std::vector<logic::attachment> &attached_open_chat() {
    if (!root_.opened || attached_for_ != *root_.opened) {
      attached_for_ = root_.opened;
      attached_.clear();
    }
    return attached_;
  }

private:
  // Given to the machine's own clipboard: what copy, share and giving a
  // conversation away all do with what they took, each of them asking for
  // no more than that. The host carries it from the scene to wherever the
  // window's own clipboard is asked for (skiff::scene::setClipboardText).
  void give(const std::string &text) {
    if (text.empty())
      return;
    scene::setClipboardText(text);
  }

  // What asked it is the model asked: which is to say nothing of what any of
  // this is, only the wire it is spoken through.
  [[nodiscard]] static moment now_ms() {
    return static_cast<moment>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
  }

  // One answer asked of a model, and the message it is being written into:
  // what asked it, what carries the bytes, and when it began, which is how
  // long an answer is said to have taken.
  //
  // Nothing outside this class knows of any of it. The window is drawn again
  // every frame from `root()`, so an answer arriving is only ever a message
  // whose text is longer than it was.
  //
  // What asked it, what carries the bytes, what of it is ours to answer, and
  // when it began, which is how long an answer is said to have taken. An
  // answer may take more than one asking: a model may stop to ask for
  // something this program owns, be answered, and go on -- so the question is
  // a loop and what is being carried now is one round of it.
  struct answer_coming {
    conversation_id chat;
    // What the answer is written into: a message by its id, and nothing at
    // all where the answer names the chat instead of saying something in it
    // (begin_title) -- nothing is made to stand in for those.
    message_id said;
    // Whether this asking names the chat rather than answering in it: the
    // words come back to the title (poll_response), and no message of
    // the chat is made or written for them.
    bool into_title = false;
    provider::loop going;
    std::optional<provider::run> asked;
    platform::carrying carried;
    tools::owned mine;
    moment began;
    // Every call this answer has stopped to ask for so far, and what each of
    // them was answered: kept here, over every round of the asking, because
    // what asked for them and what answered them are two ends of a wire and
    // neither of them is the loop between.
    std::vector<call_made> calls{};
  };
  std::optional<answer_coming> answering_;
  // What is attached to what the window has open, and which chat that was --
  // `attached_open_chat` reads both and clears on a chat of another one.
  std::vector<logic::attachment> attached_;
  std::optional<conversation_id> attached_for_;
  // What the machine's own picker was last asked for: an archive whole or
  // files to attach -- one picker stands behind both, and only the asker
  // knows which was meant (load_archive, attach_files).
  enum class files_want { nothing, archive, attach };
  files_want files_wanted_ = files_want::nothing;
  // Whether the question this window sent waits on a compact standing in
  // front of it: set when a send lands at the edge of the window
  // (send_message), and spent when the compact that stood before it finishes
  // (poll_response) -- one deep, and never two: a second send while an
  // answer is in flight does not ask at all, as it never has.
  bool pending_ask_ = false;

  // Asked of a model, where anything here can carry the asking. Today nothing
  // can anywhere but a page -- that is what `carrying::any` says -- and where
  // nothing can, nothing is asked and no answer comes, as it was before.
  void ask_model(const conversation_id &chat) {
    begin_ask(chat, told_of(chat), false);
  }

  // The door this window asks through, whole for whatever it asks: where it
  // stands -- and this page's own server where the chosen one stands
  // nowhere, as it did before any door was offered -- what opens it for that
  // door, which model answers in this chat, and what this one asking is told
  // (pchat.config, providers_offered; endpoint's `family` says whose words
  // an unknown model speaks there).
  [[nodiscard]] provider::endpoint endpoint_of(const conversation_id &chat,
                                         std::string told) const {
    const provider_choice &there = provider_by(where_.provider);
    return provider::endpoint{
        .base = !there.base.empty() ? std::string(there.base) : opened_.base,
        .key = key_of(where_, where_.provider),
        .model = ending_of(chat),
        .told = std::move(told),
        .family = std::string(there.alias)};
  }

  // One asking begun, whatever it is asked for: an answer to what was sent,
  // or a compact of what was said (compact_context, below) -- told what this
  // one is told, written into a message that is one thing or the other, and
  // carrying every control a normal answer is asked with: door, numbers,
  // thinking and tools alike (docs/en/context.md says the same of a manual
  // compact there).
  void begin_ask(const conversation_id &chat, std::string told,
                 bool capsule) {
    if (answering_)
      return;
    platform::carrying carried;
    if (!carried.any())
      return;
    const provider::endpoint at = endpoint_of(chat, std::move(told));
    // The conversation whole, as the tools read it: every message of it,
    // because a tool asked about this conversation is asked about all of it
    // -- a compacted stretch is in the chat as it always was, capsule or
    // not, and searching it finds it there.
    const std::vector<message> of_chat =
        root_.messages |
        std::views::filter(
            [&chat](const message &one) { return one.chat == chat; }) |
        std::ranges::to<std::vector<message>>();
    // What goes out on the wire: the last compact capsule and everything
    // said after it (core, wire_view) -- what stood before a capsule was
    // summarised into it, and only the wire sees the handoff. The compact
    // itself is asked over this same view, which is what makes its capsule a
    // summary of exactly what would have gone out without it.
    const std::vector<message> said = wire_view(of_chat);
    // What is ours to answer goes in the asking, so a model knows it may stop
    // and ask; and it is kept here, because answering those calls later is
    // this program's and nobody else's.
    tools::owned mine{of_chat, now_ms()};
    // How the reader has asked for it to answer, whole: the five numbers, the
    // thinking and how hard, and the tools that were left on -- both halves
    // said in one place (src/tools.cc, `wanted_of`), where before this the
    // asking carried the tools alone and none of what the settings rows move.
    const provider::wanted wants = tools::wanted_of(how_);
    // What of the conversation fits the window its answerer has: the oldest
    // message of it that is asked about, which is where the asking starts
    // below. Nothing is cut from the conversation to find it out -- the
    // messages stay whole, are drawn as they were, and are read whole by the
    // tools -- only what is written onto the wire is decided here, which is
    // what Agora's own preparation decides there (src/provider/budget.cc).
    const message_id from =
        provider::fit_from(said, at, room_of(chat), wants);
    // Put where it will stay before anything is given to it: what a platform
    // carries for is answerable at the address it was given, and a carrying
    // that is moved afterwards answers into what was left behind -- it came,
    // it answered, it was done, and the one asking never heard a word of it.
    answer_coming &coming = answering_.emplace(
        answer_coming{.chat = chat,
                      .said = message_id{chat.value + "-" +
                                         std::to_string(root_.messages.size())},
                      .going = provider::loop{
                          at,
                          said | std::views::drop_while([&from](const message &one) {
                            return one.id != from;
                          }),
                          4,
                          wants},
                      .carried = std::move(carried),
                      .mine = std::move(mine),
                      .began = now_ms()});
    // The answer as a message before anything of it has come: written into
    // as it arrives, and kept whole when nothing more is. A compact's own
    // message is named for what it is instead of for who would have said
    // it, and marked -- everything that reads messages afterwards, the wire
    // above all, then knows what it holds.
    carry(coming);
    root_.messages.push_back(
        message{.id = coming.said,
                .chat = chat,
                .author = capsule ? "Compact" : ending_of(chat),
                .outgoing = false,
                .sent = coming.began,
                .state = delivery::writing,
                .capsule = capsule});
  }

  // The chat named, as a chat gets named: asked for with the first thing
  // the reader said in it and nothing after it -- an assistant's replies and
  // later messages are not what a chat is called by (docs/en/title-
  // generation.md, read for this). Where Agora asks for it the moment the
  // first message is saved, this program asks through one wire at a time,
  // so it is asked when that wire is free (poll_response, below):
  // automatic either way, and the reader waits on it no more than they wait
  // on the answer that finished in front of it.
  void begin_title(const conversation_id &chat) {
    if (answering_)
      return;
    platform::carrying carried;
    if (!carried.any())
      return;
    // Still the name it was made with, and something has been said in it:
    // a chat called anything else has been named, and a chat with nothing
    // said has nothing to be named by.
    bool untouched = false;
    for (const conversation &room : root_.conversations)
      if (room.id == chat && room.title == new_chat_title) {
        untouched = true;
        break;
      }
    if (!untouched)
      return;
    const message *first = nullptr;
    for (const message &one : root_.messages)
      if (one.chat == chat && one.outgoing) {
        first = &one;
        break;
      }
    if (first == nullptr)
      return;
    const provider::endpoint at = endpoint_of(chat, std::string(title_told));
    const provider::wanted wants = tools::wanted_of(how_);
    const std::vector<message> one_said{*first};
    tools::owned mine{one_said, now_ms()};
    answer_coming &coming = answering_.emplace(answer_coming{
        .chat = chat,
        .said = message_id{},
        .into_title = true,
        .going = provider::loop{at, one_said, 4, wants},
        .carried = std::move(carried),
        .mine = std::move(mine),
        .began = now_ms()});
    carry(coming);
    // And nothing written into the chat: these words come back to its name
    // (poll_response), not into a message of it.
  }

  // One round of the asking given to whatever carries it here.
  void carry(answer_coming &one) {
    one.asked = one.going.answering();
    // Told that it is being carried before it is carried: nothing of an
    // answer is taken by an asking that has not been told it is going, and
    // everything that came was being dropped because nobody had said it was
    // being carried at all.
    one.asked->carried();
    one.carried.begin(one.asked->wants().url, one.asked->wants().headers,
                      one.asked->wants().body);
  }

  // What has arrived since the last frame, handed to the answer; the message
  // written again as it reads; and when nothing more is coming, the answer
  // said whole -- thought, calls, tokens and all -- in the message's place.
  // Where it stopped because it asked for something this program owns, those
  // calls are answered here and the question is asked again with them.
  void poll_response() {
    if (!answering_ || !answering_->asked)
      return;
    answer_coming &one = *answering_;
    const std::string arrived = one.carried.take();
    if (!arrived.empty())
      one.asked->arrive(arrived);
    if (message *there = found(one.said))
      there->text = std::string(one.going.said()) +
                    std::string(one.asked->said());
    if (!one.carried.over())
      return;
    one.asked->carried_over(one.carried.trouble());
    one.going.finish_run(*one.asked);
    one.asked.reset();
    if (!one.going.pending_calls().empty() && answer_tool_calls(one)) {
      carry(one);
      if (message *there = found(one.said))
        there->text = std::string(one.going.said());
      return;   // and it is not over yet
    }
    const moment ended = now_ms();
    bool capsule_written = false;
    if (one.into_title) {
      // The name, whole as the model said it and as far as a row of the
      // drawer shows it (core, title_from): written only while the chat is
      // still called what a new chat is made called -- a name set any other
      // way stands, as a manual rename stands over a request still out on
      // Agora's side (nobody here can set one yet, and when they can, this
      // is where that promise already lives). Nothing usable said: the name
      // it had, and the next answer asks again.
      if (const std::string name = title_from(one.going.said());
          !name.empty()) {
        for (conversation &room : root_.conversations)
          if (room.id == one.chat && room.title == new_chat_title) {
            room.title = name;
            break;
          }
      }
    } else if (message *there = found(one.said)) {
      // What the message was before an answer stood whole in its place: a
      // compact's capsule stays a capsule and stays called by what it is --
      // an answer is written into its message and decides nothing about
      // which message that is.
      const bool capsule = there->capsule;
      capsule_written = capsule;
      const std::string author =
          capsule ? std::string("Compact") : ending_of(one.chat);
      *there = one.going.answer_of(provider::answer_target{.id = one.said,
                                                    .chat = one.chat,
                                                    .author = author,
                                                    .sent = ended,
                                                    .took_ms = ended - one.began});
      there->capsule = capsule;
      // Those calls are the thought's to keep, once there is a thought: what
      // it wrote to itself is read beside them.
      if (there->before)
        there->before->calls = one.calls;
    }
    recount(one.chat);
    const conversation_id answered_in = one.chat;
    const bool title_ask = one.into_title;
    answering_.reset();
    // The question that waited on this compact is asked now, with the
    // capsule standing where the old stretch stood (the wire carries both
    // and the answer comes to what was sent before it). Where there is no
    // such question, the wire is free and the chat that just answered may
    // still be called what a new chat is called: its own name is asked for
    // now -- and where that asking failed or said nothing, asked for again
    // after the next answer, until it lands or the chat is named.
    if (capsule_written && pending_ask_) {
      pending_ask_ = false;
      ask_model(answered_in);
    } else if (!title_ask) {
      begin_title(answered_in);
    }
  }

  // Every call it stopped to ask for, answered by whoever owns it -- here,
  // now, by this program -- and the question asked again with what they said.
  // False where there is nothing left to ask, which is when the answer is put
  // where it belongs.
  [[nodiscard]] bool answer_tool_calls(answer_coming &one) {
    std::vector<provider::call_answered> answered;
    for (const provider::tool_call &call : one.going.pending_calls()) {
      const std::string said =
          one.mine.answered(call.name, call.arguments)
              .value_or("nothing here answers that call");
      answered.push_back(provider::call_answered{.own = call.id, .said = said});
      // Said for the fold above the answer too, and not only for the wire
      // that asked again: what it stopped to ask for, and what came of it.
      one.calls.push_back(
          call_made{.tool = call.name, .asked = call.arguments, .answered = said});
    }
    return one.going.again(std::move(answered));
  }

  // One of the messages, by its answer's name: nothing where it is not one.
  [[nodiscard]] message *found(const message_id &said) {
    for (message &one : root_.messages)
      if (one.id == said)
        return &one;
    return nullptr;
  }
  // Which model answers in a chat, named as the messages name it.
  [[nodiscard]] std::string ending_of(const conversation_id &chat) const {
    const std::string model = model_of(chat);
    return model.empty() ? std::string("Model") : model;
  }

  void recolour() {
    dark_ = dark_of(shown_.mode, true);
    look_ = ui::paints_of(colors_of(shown_, true), dark_);
    look_.size = ui::body_size * static_cast<float>(shown_.font_scale);
  }

  // Where in the messages one said is -- one past its own place, so that a
  // fork copies everything said before it.
  [[nodiscard]] std::size_t message_index(const message_id &said) const {
    for (std::size_t at = 0; at < root_.messages.size(); ++at)
      if (root_.messages[at].id == said)
        return at + 1;
    return 0;
  }
  // What one said reads, or nothing where there is no such said.
  [[nodiscard]] std::string text_of(const message_id &said) const {
    for (const message &one : root_.messages)
      if (one.id == said)
        return one.text;
    return {};
  }
  // The conversation this window has open, where one of them is open: what
  // a press that is about the open chat changes, and nothing else.
  [[nodiscard]] conversation *opened_conversation() {
    if (!root_.opened)
      return nullptr;
    for (conversation &one : root_.conversations)
      if (one.id == *root_.opened)
        return &one;
    return nullptr;
  }
  // Which model answers in a chat, as the model keeps it.
  [[nodiscard]] std::string model_of(const conversation_id &chat) const {
    for (const conversation &one : root_.conversations)
      if (one.id == chat)
        return one.leading_model();
    return {};
  }
  // What every answerer in a chat is told before anything is said to it.
  [[nodiscard]] std::string told_of(const conversation_id &chat) const {
    for (const conversation &one : root_.conversations)
      if (one.id == chat)
        return one.told_for();
    return std::string(default_told);
  }
  // How much room this chat's answerer has to be asked in: what the window of
  // the conversation says, and none where no conversation is open -- which
  // cuts nothing of anything, because nobody has said how much room there is.
  [[nodiscard]] std::uint32_t room_of(const conversation_id &chat) const {
    for (const conversation &one : root_.conversations)
      if (one.id == chat)
        return one.window;
    return 0;
  }
  // What this chat has spent of the room its answerer has: said again whole
  // after any message enters or leaves it, and after the window changes, so
  // that what the foot's circle reads is the messages added up.
  void recount(const conversation_id &chat) {
    conversation *one = nullptr;
    for (conversation &each : root_.conversations)
      if (each.id == chat)
        one = &each;
    if (!one)
      return;
    // What the next asking would carry, counted: the chat keeps everything
    // it ever had, but what it has spent of its window is what the wire
    // would send -- a compacted stretch stands in its capsule's tokens now,
    // and the circle over the field says so (core, wire_view).
    std::vector<message> of_chat;
    for (const message &each : root_.messages)
      if (each.chat == chat)
        of_chat.push_back(each);
    std::uint32_t spent = 0;
    for (const message &each : wire_view(of_chat))
      spent += each.tokens;
    one->tokens = spent;
    one->context_used =
        one->window == 0
            ? 0.0f
            : static_cast<float>(spent) / static_cast<float>(one->window);
  }

  // Whether the next asking of this chat would overflow the window it has:
  // the chat's own share of the budget (core, overruns -- Agora's
  // automatic-compact point), read from what recount just said of it.
  [[nodiscard]] bool overruns_of(const conversation_id &chat) const {
    for (const conversation &room : root_.conversations)
      if (room.id == chat)
        return overruns(room.window, room.context_used);
    return false;
  }

  // What one said and everything before it in its chat was worth.
  [[nodiscard]] std::uint32_t tokens_up_to(const conversation_id &chat,
                                           const message_id &said) const {
    std::uint32_t out = 0;
    for (const message &one : root_.messages) {
      if (one.chat != chat)
        continue;
      out += one.tokens;
      if (one.id == said)
        break;
    }
    return out;
  }
  // This one away, and every one after it in its chat: what rub out and ask
  // again both come down to, the one leaving the question above it to be
  // answered again.
  void cut_away(const message_id &said) {
    const std::size_t found = message_index(said);
    if (found == 0)
      return;
    const std::size_t at = found - 1;
    const conversation_id chat = root_.messages[at].chat;
    std::vector<message> kept;
    kept.reserve(root_.messages.size());
    for (std::size_t one = 0; one < root_.messages.size(); ++one)
      if (one < at || root_.messages[one].chat != chat)
        kept.push_back(root_.messages[one]);
    root_.messages = std::move(kept);
    recount(chat);
  }

  // The door a model is spoken through, while this is only a client: the
  // server that served the page is the one expected to answer it, which is
  // why the way in is written relative and why there is no key in it -- whose
  // a key is, is that server's own to know.
  const provider::endpoint opened_{
      .base = "/v1",
      .told = "You are answering in Progressive Chat, briefly and plainly."};
  window_scene scene_{std::in_place};
  chats_root root_;
  looks shown_;
  asking how_;
  // Which door the asking goes through, and what opens each of them: said
  // with the settings, read back at the start of the next run, and drawn by
  // the page that chooses it (src/ui/settings.cc, say_doors).
  door where_;
  ui::paints look_;
  requests asked_;
  // The machine's own file dialogs, where they are asked for: the program's,
  // put here when it is made (dialogs_from), and null until then.
  skiff::platform::dialogs::dialogs *dialogs_ = nullptr;
  std::string query_;
  bool dark_ = true;
  bool settings_ = false;
  // The drawer is beside the messages, where the window has room for both,
  // and it comes over them only where it is asked for with the pill. It was
  // asked for from the first frame onwards, which a wide window answered by
  // showing it beside them -- and a narrower one, or a phone, by showing it
  // over them and the conversation not at all: nothing was left in the middle
  // of the window but the drawer's own ground.
  bool drawer_ = false;
  // Whether the design said in the room is being read.
  bool design_ = false;
  // Whether what every model is told in this conversation is being read and
  // written: one card, over everything, for the conversation that is open.
  bool system_ = false;
  // Whether what stands behind the three dots is out: put away again by one
  // press of anything it holds, or of the dots themselves.
  bool more_ = false;
  // Whether what stands behind the card's three dots is out: thinking, how
  // hard it is asked to think, and the way into the large settings (the
  // design says them under *The editor's row*).
  bool editor_menu_ = false;
  // Whether the card of faces above the face beside the clip is out.
  bool faces_ = false;
  // The faces pressed most lately, newest first, and no more of them than
  // the card's own row holds: drawn above the rest of them, which is what
  // they were kept for (and what `pchat.config`'s `recent_emoji` is for,
  // once there are runs to keep them between).
  std::vector<std::string> faces_lately_{};
  moment said_count_ = 1;
  std::size_t chat_count_ = 0;

  float width_ = 1100.0f;
  float height_ = 720.0f;
};

} // namespace pchat::app
