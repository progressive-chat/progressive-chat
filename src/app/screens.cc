// SPDX-License-Identifier: AGPL-3.0-only
// pchat.app.screens -- What the window shows: one model, one set of looks,
// one scene. Control requests are answered here between frames, and the tree
// is filled in from the model before each frame.
//
// Nothing here talks to a server: the sample fills the model until the
// client does.
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

// The tree the host draws, held by a Scene. Declared here so the program
// needs nothing of the screens but these.
using window_scene = scene::Scene<ui::window_type>;

// Everything shown, and what is done with what was asked for.
class screens {
public:
  screens()
      : root_(sample_chats()), shown_(default_looks),
        look_(ui::paints_of(colors_of(shown_, true), true)) {}

  [[nodiscard]] window_scene &window() noexcept { return scene_; }

  // How answers are asked for now: one of the five, kept inside its lines,
  // as a settings row may move it.
  [[nodiscard]] const asking &how() const noexcept { return how_; }
  void how(asking_what field, double value) { how_ = set_of(how_, field, value); }

  // Answered between frames: what the controls asked for, oldest first.
  void take(const request &one) {
    spl::visit(
        spl::overloaded{
            [&](const open_conversation &asking) {
              root_.opened = asking.chat;
              // The card of faces belongs to this conversation: closing it
              // keeps it here rather than following the reader elsewhere.
              faces_ = false;
              // A drawer opened over the messages has been used: put away,
              // leaving the conversation it opened.
              if (width_ < ui::side_by_side_from)
                drawer_ = false;
            },
            [&](const send_message &asking) {
              const logic::sent_message said = logic::send(
                  logic::sending_request{
                      asking.chat, asking.text, {}, std::nullopt},
                  "Reader", static_cast<moment>(said_count_++));
              root_.messages.push_back(said.said);
              recount(asking.chat);
              ask_a_model(asking.chat);
              // The sent text left the field, and the card that writes into
              // it goes with it: nothing left open can write into a field
              // that was already given away.
              faces_ = false;
            },

            [&](const new_chat &) {
              const conversation_id made{"chat-" +
                                         std::to_string(++chat_count_)};
              conversation fresh{
                  made,
                  root_.accounts.empty() ? account_id{}
                                         : root_.accounts.front().id,
                  "New chat", conversation_kind::direct, ++said_count_};
              if (root_.opened)
                fresh.models.push_back(model_of(*root_.opened));
              // A new chat takes the whole window: all of its leading
              // model's window to talk into.
              fresh.window = window_of(fresh.leading_model());
              root_.conversations.insert(root_.conversations.begin(),
                                         std::move(fresh));
              root_.opened = made;
              if (width_ < ui::side_by_side_from)
                drawer_ = false;
            },
            // Taken whole, then put down somewhere else.
            [&](const copy_answer &asking) { this->give(text_of(asking.said)); },
            // One line of the settings page: what it says is given whole.
            [&](const copy_text &asking) { this->give(asking.text); },
            // This answer and every one after it are removed, and the
            // question above them is asked again at once. Before, only the
            // answer was removed: each press cost the conversation an answer
            // and gave none back, though the asker was already here and this
            // line alone kept it out.
            [&](const regenerate_answer &asking) {
              cut_away(asking.said);
              ask_a_model(asking.chat);
            },
            // Into a chat of its own: everything asked and answered up to
            // this answer, nothing said after it.
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
              const std::size_t up_to = said_at(asking.said);
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
            // Given away as it reads, until the program has somewhere to
            // give it to.
            [&](const share_answer &asking) { this->give(text_of(asking.said)); },
            [&](const delete_answer &asking) { cut_away(asking.said); },
            [&](const set_query &asking) { query_ = asking.text; },
            [&](const toggle_drawer &) { drawer_ = !drawer_; },
            // Settings open from the card's own three dots as well as from
            // the drawer, and what opened them is put away either way.
            [&](const open_settings &) {
              settings_ = true;
              editor_menu_ = false;
            },
            [&](const close_settings &) { settings_ = false; },
            // The drawer's two rows: nothing stands behind either of them
            // yet, so neither opens. They are drawn where the panel has them,
            // and what they ask for is answered once something can answer it.
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
            // Given away as it reads, until the program has somewhere to
            // give it to: everything said in the conversation, in order.
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
            // What the card's three dots hold: thinking, how hard it is
            // asked to think, and the large settings. Thinking and effort
            // set how every answer in every conversation is asked for from
            // now on (pchat.config, `asking`), which the settings' own rows
            // would also have said.
            [&](const toggle_editor_menu &) { editor_menu_ = !editor_menu_; },
            // The card of faces above the face beside the clip: opened by a
            // press on it, put away by another press anywhere else in the
            // room, which nothing measured asks for (src/ui/window.cc).
            [&](const toggle_faces &) { faces_ = !faces_; },
            // One face pressed: moved to the front of the ones pressed
            // lately, wherever it stood before, and no longer a row than the
            // card draws. Kept in press order, not by count, so the first is
            // the one wanted last, not the one wanted most often.
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
            // and written whole, kept by the conversation as it is typed.
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
            // putting the design away leaves the conversation and not a page
            // the reader had left.
            [&](const open_design &) {
              settings_ = false;
              design_ = true;
            },
            [&](const close_design &) { design_ = false; },
            // What the seed is: every colour grown again from it, and the
            // page staying open to look at.
            [&](const pick_seed &asking) {
              shown_.preset = asking.preset;
              recolour();
            },
            [&](const set_amoled &asking) {
              shown_.amoled = asking.enabled;
              recolour();
            },
            // What the drawer's rows are then drawn with: no colour grows
            // out of either of these, so nothing is grown again.
            [&](const set_avatars &asking) { shown_.avatars = asking.enabled; },
            [&](const set_row_times &asking) {
              shown_.row_times = asking.enabled;
            },
            // A test notification: nothing here can know whether the machine
            // will show it, so this only asks the platform and goes on.
            [&](const send_test_notification &) {
              platform::notified::say("progressive-chat",
                                      "This is a test notification.");
            },
            // Larger type: every size drawn in a message's body is asked
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
            // pressed for, kept inside its lines (pchat.config, `asking`).
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
            // The archive asked for: which files are kept where is the
            // machine's own to say; this only names the file. The bytes are
            // written when a path comes back (write_archive, through the
            // program).
            [&](const save_archive &) {
              if (dialogs_)
                dialogs_->choose_save_path("progressive-chat.prgrssv");
            },
            // An answerer put into the open conversation or taken out of
            // it: the window then writes to the first one left, and its
            // window is that model's own.
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
    // A link pressed in a message since the last frame: skiff names the
    // ones pressed, as they stand in the text they were drawn in, and each
    // goes to the machine to open -- a browser on a desk and a phone, a
    // new tab where this is a page. What opens is not this program's to know.
    for (const std::string &place : std::exchange(scene::hostWork().links, {}))
      platform::opened::open(place);
    read_what_arrived();
    scene_.root().again(look_, root_, shown_, how_, query_, dark_,
                        ui::what_is_open{.settings = settings_,
                                         .design = design_,
                                         .prompt = system_,
                                         .more = more_,
                                         .editor_menu = editor_menu_,
                                         .faces = faces_,
                                         .drawer = drawer_},
                        faces_lately_, width_, height_, &asked_);
  }

  [[nodiscard]] chats_root &root() noexcept { return root_; }
  [[nodiscard]] const chats_root &root() const noexcept { return root_; }
  [[nodiscard]] bool dark() const noexcept { return dark_; }

  // Where the machine's file dialogs are asked for: the program holds them
  // (they answer on its thread), and this screen knows a save is wanted and
  // what the file is called. Nothing here is a dialog -- the chosen path
  // comes back through the program (save_path_chosen).
  void dialogs_from(skiff::platform::dialogs::dialogs &one) {
    dialogs_ = &one;
  }

  // Where the machine said the archive stands: the bytes built whole and
  // given to the platform once -- a page turns the file written there into a
  // download under its own name, a desk leaves it where it was asked to
  // stand (skiff.platform.storage).
  void write_archive(const std::string &path) {
    skiff::platform::storage::write_file(
        path, prgrssv::archive_of(root_, shown_, how_));
  }

private:
  // Given to the machine's clipboard: what copy, share and giving a
  // conversation away all do with what they took, each asking no more than
  // that. The host carries it from the scene to the window's own clipboard
  // (skiff::scene::setClipboardText).
  void give(const std::string &text) {
    if (text.empty())
      return;
    scene::setClipboardText(text);
  }

  // The moment, taken from the system clock: milliseconds since the epoch,
  // the only wire time this knows.
  [[nodiscard]] static moment now_ms() {
    return static_cast<moment>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count());
  }

  // One answer asked of a model, and the message it is written into: what
  // asked it, what carries the bytes, and when it began -- the last is how
  // long an answer is said to have taken.
  //
  // Nothing outside this class knows of any of it. The window is drawn
  // again every frame from `root()`, so an arriving answer is only ever a
  // message whose text is longer than it was.
  //
  // An answer may take more than one asking: a model may stop to ask for
  // something this program owns, be answered, and go on. The question is
  // then a loop, and what is being carried now is one round of it.
  struct answer_coming {
    conversation_id chat;
    message_id said;
    provider::loop going;
    std::optional<provider::run> asked;
    platform::carrying carried;
    tools::owned mine;
    moment began;
    // Every call this answer has stopped to ask for so far, and what each
    // was answered: kept here across every round, because what asked for
    // them and what answered them are two ends of a wire and neither is the
    // loop between.
    std::vector<call_made> calls{};
  };
  std::optional<answer_coming> answering_;

  // Asked of a model, where anything here can carry the asking. Today only
  // a page can -- that is what `carrying::any` says -- and where nothing
  // can, nothing is asked and no answer comes.
  void ask_a_model(const conversation_id &chat) {
    if (answering_)
      return;
    platform::carrying carried;
    if (!carried.any())
      return;
    const provider::endpoint at{.base = opened_.base,
                                .model = ending_of(chat),
                                .told = told_of(chat)};
    // Not const: a view over messages is asked of more than once and read
    // more than once, and a const view says no to a second reading.
    auto turns = root_.messages |
                       std::views::filter([&chat](const message &one) {
                         return one.chat == chat;
                       });
    // What is ours to answer goes in the asking, so a model knows it may
    // stop and ask; it is kept here because answering those calls later is
    // this program's alone.
    tools::owned mine{turns, now_ms()};
    // Put where it will stay before anything is given to it: what a platform
    // carries for answers at the address it was given, and a carrying moved
    // afterwards answers into what was left behind. It came, it answered, it
    // was done, and the one asking never heard of it.
    answer_coming &coming = answering_.emplace(
        answer_coming{.chat = chat,
                      .said = message_id{chat.value + "-" +
                                         std::to_string(root_.messages.size())},
                      .going = provider::loop{at,
                                              turns,
                                              4,
                                              provider::wanted{
                                                  .tools = mine.offered()}},
                      .carried = std::move(carried),
                      .mine = std::move(mine),
                      .began = now_ms()});
    // The answer as a message before anything of it has come: written into as
    // it arrives, and kept whole when nothing more is.
    carry(coming);
    root_.messages.push_back(message{.id = coming.said,
                                     .chat = chat,
                                     .author = ending_of(chat),
                                     .outgoing = false,
                                     .sent = coming.began,
                                     .state = delivery::writing});
  }

  // One round of the asking given to what carries it here.
  void carry(answer_coming &one) {
    one.asked = one.going.answering();
    // Told it is being carried before it is carried: no answer is taken by
    // an asking that was never told it is going. Everything that came used
    // to be dropped because nothing said it was being carried.
    one.asked->carried();
    one.carried.begin(one.asked->wants().url, one.asked->wants().headers,
                      one.asked->wants().body);
  }

  // What has arrived since the last frame, handed to the answer; the message
  // written again as it reads; and when nothing more is coming, the answer
  // said whole -- thought, calls, tokens and all -- in the message's place.
  // Where it stopped because it asked for something this program owns, those
  // calls are answered here and the question is asked again with them.
  void read_what_arrived() {
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
    one.going.came_back(*one.asked);
    one.asked.reset();
    if (!one.going.asked_for().empty() && asked_again(one)) {
      carry(one);
      if (message *there = found(one.said))
        there->text = std::string(one.going.said());
      return;   // the answer is not over yet
    }
    const moment ended = now_ms();
    if (message *there = found(one.said)) {
      *there = one.going.answer_of(provider::put_in{.id = one.said,
                                                    .chat = one.chat,
                                                    .author = ending_of(one.chat),
                                                    .sent = ended,
                                                    .took_ms = ended - one.began});
      // Those calls are the thought's to keep, once there is a thought:
      // what it wrote to itself is read beside them.
      if (there->before)
        there->before->calls = one.calls;
    }
    recount(one.chat);
    answering_.reset();
  }

  // Every call it stopped to ask for, answered by whoever owns it -- here,
  // now, by this program -- and the question asked again with what they said.
  // False where there is nothing left to ask, which is when the answer is put
  // where it belongs.
  [[nodiscard]] bool asked_again(answer_coming &one) {
    std::vector<provider::call_answered> answered;
    for (const provider::tool_call &call : one.going.asked_for()) {
      const std::string said =
          one.mine.answered(call.name, call.arguments)
              .value_or("nothing here answers that call");
      answered.push_back(provider::call_answered{.own = call.id, .said = said});
      // Said for the fold above the answer too, not only for the wire that
      // asked again: what it stopped to ask for, and what came of it.
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

  // Where in the messages one said is -- one past its own place, so a fork
  // copies everything said before it.
  [[nodiscard]] std::size_t said_at(const message_id &said) const {
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
  // The conversation this window has open, where one is open: what a press
  // about the open chat changes, and nothing else.
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
  // What this chat has spent of its answerer's room: said again whole after
  // any message enters or leaves it, and after the window changes, so the
  // foot's circle reads the messages added up.
  void recount(const conversation_id &chat) {
    conversation *one = nullptr;
    for (conversation &each : root_.conversations)
      if (each.id == chat)
        one = &each;
    if (!one)
      return;
    std::uint32_t spent = 0;
    for (const message &each : root_.messages)
      if (each.chat == chat)
        spent += each.tokens;
    one->tokens = spent;
    one->context_used =
        one->window == 0
            ? 0.0f
            : static_cast<float>(spent) / static_cast<float>(one->window);
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
  // again both come down to, the question above the message open to be
  // answered again.
  void cut_away(const message_id &said) {
    const std::size_t found = said_at(said);
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
  // server that served the page is expected to answer it, which is why the
  // way in is relative and why there is no key in it. Whose a key is is
  // that server's own to know.
  const provider::endpoint opened_{
      .base = "/v1",
      .told = "You are answering in Progressive Chat, briefly and plainly."};
  window_scene scene_{std::in_place};
  chats_root root_;
  looks shown_;
  asking how_;
  ui::paints look_;
  requests asked_;
  // The machine's file dialogs, where they are asked for: the program's,
  // put here when it is made (dialogs_from), and null until then.
  skiff::platform::dialogs::dialogs *dialogs_ = nullptr;
  std::string query_;
  bool dark_ = true;
  bool settings_ = false;
  // The drawer is beside the messages where the window has room for both,
  // and comes over them only where the pill asks for it. It was asked for
  // from the first frame onwards: a wide window answered by showing it
  // beside them, a narrower one or a phone by showing it over them and the
  // conversation not at all, leaving nothing in the middle of the window
  // but the drawer's own ground.
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
  // The faces pressed most lately, newest first, no more than the card's
  // own row holds: drawn above the rest, which is what they were kept for
  // (and what `pchat.config`'s `recent_emoji` is for, once there are runs
  // to keep them between).
  std::vector<std::string> faces_lately_{};
  moment said_count_ = 1;
  std::size_t chat_count_ = 0;

  float width_ = 1100.0f;
  float height_ = 720.0f;
};

} // namespace pchat::app
