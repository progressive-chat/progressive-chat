// SPDX-License-Identifier: AGPL-3.0-only
// pchat.prgrssv -- Everything the program holds, written whole into one file:
// a ZIP archive under the extension .prgrssv, so that what was said here can be
// kept, carried, and read where this program is not running.
//
// Three kinds of file stand in it: a manifest saying what the archive is and
// what it holds, one file per conversation -- every message, what thought it,
// and what its calls asked and got -- and the settings the rows of the page
// have set. What puts those files into a ZIP is the classic side
// (src/prgrssv_zip.cpp), where the port's writer stays as the port's headers
// stay everywhere else (src/provider/anthropic/wire.h, the same rule).
module;
#include "prgrssv_zip.h"

export module pchat.prgrssv;

import std;
import pchat.core;
import pchat.config;
import pchat.theme;
import pchat.json;

// What each of the model's own words is called in an archive: one enumerator
// to one name, so that nothing written here reads back as something else.
// Said beside the export and not inside it: what an exported definition
// reaches for may not be translation-unit local, and an anonymous namespace
// cannot be exported at all.
namespace pchat::prgrssv {

[[nodiscard]] std::string_view said_of(conversation_kind one) {
  switch (one) {
  case conversation_kind::direct:
    return "direct";
  case conversation_kind::group:
    return "group";
  case conversation_kind::channel:
    break;
  }
  return "channel";
}

[[nodiscard]] std::string_view said_of(delivery one) {
  switch (one) {
  case delivery::writing:
    return "writing";
  case delivery::sending:
    return "sending";
  case delivery::sent:
    return "sent";
  case delivery::failed:
    return "failed";
  case delivery::seen:
    break;
  }
  return "seen";
}

[[nodiscard]] std::string_view said_of(theme_mode one) {
  switch (one) {
  case theme_mode::light:
    return "light";
  case theme_mode::dark:
    return "dark";
  case theme_mode::system:
    break;
  }
  return "system";
}

[[nodiscard]] std::string_view said_of(effort one) {
  switch (one) {
  case effort::least:
    return "least";
  case effort::some:
    return "some";
  case effort::most:
    return "most";
  case effort::all:
    break;
  }
  return "all";
}

[[nodiscard]] std::string_view said_of(font_choice one) {
  switch (one) {
  case font_choice::app:
    return "app";
  case font_choice::system:
    return "system";
  case font_choice::custom:
    break;
  }
  return "custom";
}

[[nodiscard]] std::string_view said_of(scheme_preset one) {
  switch (one) {
  case scheme_preset::midnight:
    return "midnight";
  case scheme_preset::nordic:
    return "nordic";
  case scheme_preset::forest:
    return "forest";
  case scheme_preset::sunset:
    return "sunset";
  case scheme_preset::rose:
    return "rose";
  case scheme_preset::lavender:
    return "lavender";
  case scheme_preset::slate:
    return "slate";
  case scheme_preset::ocean:
    break;
  }
  return "ocean";
}

[[nodiscard]] std::string_view said_of(scheme_style one) {
  switch (one) {
  case scheme_style::tonal_spot:
    return "tonal_spot";
  case scheme_style::expressive:
    return "expressive";
  case scheme_style::vibrant:
    return "vibrant";
  case scheme_style::neutral:
    break;
  }
  return "neutral";
}

// What the tools and the switches are called, in the order their rows ask for
// them (pchat.config): the same words a settings row says and the program
// keeps, so that an archive read back lines up with what the page shows.
inline constexpr std::array<std::string_view, tools_owned> tool_names{
    "conversation_search", "how_much", "when_it_is"};
inline constexpr std::array<std::string_view, switches_owned> switch_names{
    "picture_to_words", "words_to_picture", "proxy",
    "own_page",         "remember",         "skills", "data_held"};


// ---- What a file of an archive reads back as: the same words said_of wrote,
// turned into the enums they came from, and every word, number and switch
// taken with the default it had where a file says nothing of it.

conversation_kind conversation_kind_of(std::string_view said) {
  if (said == "direct")
    return conversation_kind::direct;
  if (said == "group")
    return conversation_kind::group;
  return conversation_kind::channel;
}

delivery delivery_of(std::string_view said) {
  if (said == "writing")
    return delivery::writing;
  if (said == "sending")
    return delivery::sending;
  if (said == "sent")
    return delivery::sent;
  if (said == "failed")
    return delivery::failed;
  return delivery::seen;
}

theme_mode mode_of(std::string_view said) {
  if (said == "light")
    return theme_mode::light;
  if (said == "dark")
    return theme_mode::dark;
  return theme_mode::system;
}

effort effort_of(std::string_view said) {
  if (said == "least")
    return effort::least;
  if (said == "some")
    return effort::some;
  if (said == "most")
    return effort::most;
  return effort::all;
}

font_choice font_of(std::string_view said) {
  if (said == "app")
    return font_choice::app;
  if (said == "system")
    return font_choice::system;
  return font_choice::custom;
}

scheme_preset preset_of(std::string_view said) {
  if (said == "midnight")
    return scheme_preset::midnight;
  if (said == "nordic")
    return scheme_preset::nordic;
  if (said == "forest")
    return scheme_preset::forest;
  if (said == "sunset")
    return scheme_preset::sunset;
  if (said == "rose")
    return scheme_preset::rose;
  if (said == "lavender")
    return scheme_preset::lavender;
  if (said == "slate")
    return scheme_preset::slate;
  return scheme_preset::ocean;
}

scheme_style style_of(std::string_view said) {
  if (said == "tonal_spot")
    return scheme_style::tonal_spot;
  if (said == "expressive")
    return scheme_style::expressive;
  if (said == "vibrant")
    return scheme_style::vibrant;
  return scheme_style::neutral;
}

// What a file holds under one name: a word, a number, or what is on -- nothing
// where it says nothing of it, and the default the program had before this
// archive was written beside each of them.
[[nodiscard]] std::string text_in(const json::value &one, std::string_view name) {
  const json::value *said = json::said(one, name);
  if (said == nullptr)
    return std::string{};
  const auto got = json::text_of(*said);
  return got ? std::string(*got) : std::string{};
}

[[nodiscard]] bool truth_in(const json::value &one, std::string_view name,
                            bool fallback = false) {
  const json::value *said = json::said(one, name);
  if (said == nullptr)
    return fallback;
  const auto got = json::truth_of(*said);
  return got ? *got : fallback;
}

[[nodiscard]] double decimal_in(const json::value &one, std::string_view name,
                                double fallback = 0.0) {
  const json::value *said = json::said(one, name);
  if (said == nullptr)
    return fallback;
  const auto got = json::number_of(*said);
  return got ? *got : fallback;
}

[[nodiscard]] std::int64_t number_in(const json::value &one, std::string_view name,
                                     std::int64_t fallback = 0) {
  return static_cast<std::int64_t>(
      decimal_in(one, name, static_cast<double>(fallback)));
}

}  // namespace pchat::prgrssv

export namespace pchat::prgrssv {

// What every archive calls itself, and how far it has got: said in the
// manifest, so that anything reading one file knows what the rest of them are
// written as.
inline constexpr std::string_view format = "prgrssv";
inline constexpr std::int64_t version = 1;

// One archive, whole: what the program holds said into the files it is kept
// in, and those files zipped into the bytes a save is handed.
//
// What door it was spoken through stands in the settings with the rest of
// them -- and which key opens that door stands there too, because this is the
// file a run is kept in and read back from. A save asked to be handed
// around asks for no door at all: given the default one, its settings say
// which door was chosen and no key of anybody's, which is what makes an
// archive safe to give away.
[[nodiscard]] inline std::string archive_of(const chats_root &root,
                                            const looks &shown,
                                            const asking &how,
                                            const door &where = door{}) {
  std::vector<stored> files;

  // The manifest: what this archive is, whose program wrote it, who its
  // conversations were spoken through, and how much of everything stands in
  // it -- the index anything reading the file opens first.
  json::writing out;
  out.thing();
  out.name("format").text(format);
  out.name("version").number(version);
  out.name("program").text(std::string("progressive-chat ") + PCHAT_VERSION_SAID);
  out.name("accounts").list();
  for (const account &one : root.accounts) {
    out.thing();
    out.name("id").text(one.id.value);
    out.name("name").text(one.name);
    out.name("server").text(one.server);
    out.shut();
  }
  out.shut();
  out.name("conversations").number(
      static_cast<std::int64_t>(root.conversations.size()));
  out.name("messages").number(static_cast<std::int64_t>(root.messages.size()));
  // Which conversation the window had open: the one file that knows it all,
  // and the one thing a reader would otherwise have to guess.
  out.name("opened").text(root.opened ? root.opened->value : std::string{});
  out.shut();
  files.push_back(stored{"manifest.json", out.take()});

  // One file per conversation: the row of the drawer whole, and every message
  // in it in the order it was said -- what said each, what thought an answer
  // before it, and what each of that thought's calls asked and got.
  for (const conversation &room : root.conversations) {
    json::writing written;
    written.thing();
    written.name("id").text(room.id.value);
    written.name("account").text(room.account.value);
    written.name("title").text(room.title);
    written.name("kind").text(said_of(room.kind));
    written.name("last_activity").number(room.last_activity);
    written.name("unread").number(static_cast<std::int64_t>(room.unread));
    written.name("pinned").truth(room.pinned);
    written.name("muted").truth(room.muted);
    written.name("models").list();
    for (const std::string &model : room.models)
      written.text(model);
    written.shut();
    written.name("told").text(room.told);
    written.name("tokens").number(static_cast<std::int64_t>(room.tokens));
    written.name("window").number(static_cast<std::int64_t>(room.window));
    written.name("context_used").number(static_cast<double>(room.context_used));
    written.name("messages").list();
    for (const message &one : root.messages) {
      if (one.chat != room.id)
        continue;
      written.thing();
      written.name("id").text(one.id.value);
      written.name("author").text(one.author);
      written.name("outgoing").truth(one.outgoing);
      written.name("text").text(one.text);
      written.name("sent").number(one.sent);
      written.name("state").text(said_of(one.state));
      written.name("tokens").number(static_cast<std::int64_t>(one.tokens));
      if (one.replies_to)
        written.name("replies_to").text(one.replies_to->value);
      if (one.before) {
        written.name("thought").thing();
        written.name("took_ms").number(one.before->took_ms);
        written.name("tool_calls").number(
            static_cast<std::int64_t>(one.before->tool_calls));
        written.name("said").text(one.before->said);
        written.name("calls").list();
        for (const call_made &call : one.before->calls) {
          written.thing();
          written.name("tool").text(call.tool);
          written.name("asked").text(call.asked);
          written.name("answered").text(call.answered);
          written.shut();
        }
        written.shut();  // calls
        written.shut();  // thought
      }
      written.shut();  // message
    }
    written.shut();  // messages
    written.shut();  // conversation
    files.push_back(
        stored{"conversations/" + room.id.value + ".json", written.take()});
  }

  // The settings the rows of the page have set: how it looks and how an answer
  // is asked for, each said whole -- what was on the switches and the knobs
  // when this archive was written.
  json::writing settings;
  settings.thing();
  settings.name("looks").thing();
  settings.name("preset").text(said_of(shown.preset));
  settings.name("style").text(said_of(shown.style));
  settings.name("mode").text(said_of(shown.mode));
  settings.name("amoled").truth(shown.amoled);
  settings.name("reduce_motion").truth(shown.reduce_motion);
  settings.name("avatars").truth(shown.avatars);
  settings.name("row_times").truth(shown.row_times);
  settings.name("font_scale").number(shown.font_scale);
  settings.name("font").text(said_of(shown.font));
  settings.shut();
  settings.name("asking").thing();
  settings.name("thinking").truth(how.thinking);
  settings.name("how_hard").text(said_of(how.how_hard));
  settings.name("temperature").number(how.temperature);
  settings.name("top_p").number(how.top_p);
  settings.name("max_tokens").number(static_cast<std::int64_t>(how.max_tokens));
  settings.name("presence").number(how.presence);
  settings.name("often").number(how.often);
  settings.name("tools").thing();
  for (std::size_t at = 0u; at < tools_owned; ++at)
    settings.name(tool_names[at]).truth(how.tools[at]);
  settings.shut();
  settings.name("switches").thing();
  for (std::size_t at = 0u; at < switches_owned; ++at)
    settings.name(switch_names[at]).truth(how.switches[at]);
  settings.shut();
  settings.shut();  // asking
  // The door the next run asks through: which of the providers stands open,
  // and what opens it. Said where the rest of the settings are, because that
  // is where it is read back from -- see archive_of's note on the default.
  settings.name("door").thing();
  settings.name("provider").text(where.provider);
  settings.name("keys").list();
  for (const door_key &one : where.keys) {
    settings.thing();
    settings.name("provider").text(one.provider);
    settings.name("key").text(one.key);
    settings.shut();
  }
  settings.shut();  // keys
  settings.shut();  // door
  settings.shut();  // looks-and-asking
  files.push_back(stored{"settings.json", settings.take()});

  return zipped(files);
}


// Everything an archive held, back where it is held: the conversations with
// their messages, what the page looked like, and how an answer was asked for
// -- and why it could not be read, whole, where it could not be.
struct restored {
  chats_root root;
  looks shown;
  asking how;
  // What it asked through, back too: the door its settings named, and what
  // opens every door it ever kept a key for -- each key read back the whole
  // way it was written, which is only ever into this machine's own keeping
  // (see archive_of).
  door where;
  std::string trouble;
  [[nodiscard]] bool ok() const { return trouble.empty(); }
};

// One archive whole -- the bytes archive_of wrote -- read back into everything
// above: what a program held at the end of one run, said into the start of the
// next. Not a file's business: the bytes are given, and what they were not is
// `trouble`, empty only where every file of it was read as it was written.
[[nodiscard]] inline restored restore_of(std::string_view bytes) {
  restored out;
  std::vector<stored> files;
  if (!unzipped(bytes, files)) {
    out.trouble = "those bytes are not an archive this program reads";
    return out;
  }
  const stored *manifest = nullptr;
  for (const stored &one : files)
    if (one.name == "manifest.json") {
      manifest = &one;
      break;
    }
  if (manifest == nullptr) {
    out.trouble = "the archive holds no manifest";
    return out;
  }
  const auto said = json::read(manifest->body);
  if (!said) {
    out.trouble = "the manifest is not written in JSON";
    return out;
  }
  if (text_in(*said, "format") != format) {
    out.trouble = "the manifest does not say it is a prgrssv archive";
    return out;
  }
  if (number_in(*said, "version") > version) {
    out.trouble = "written by a later build than this one, and not read here";
    return out;
  }
  if (const std::string open = text_in(*said, "opened"); !open.empty())
    out.root.opened = conversation_id{open};
  if (const json::value *accounts = json::said(*said, "accounts"))
    for (std::size_t at = 0;; ++at) {
      const json::value *one = json::item_of(*accounts, at);
      if (one == nullptr)
        break;
      out.root.accounts.push_back(account{.id = account_id{text_in(*one, "id")},
                                          .name = text_in(*one, "name"),
                                          .server = text_in(*one, "server")});
    }

  // One conversation at a time: the drawer's own row whole, and every message
  // in it in the order its file says it.
  for (const stored &one : files) {
    if (!one.name.starts_with("conversations/") || !one.name.ends_with(".json"))
      continue;
    const auto said_room = json::read(one.body);
    if (!said_room) {
      out.trouble = one.name + " is not written in JSON";
      return out;
    }
    conversation room;
    room.id = conversation_id{text_in(*said_room, "id")};
    room.account = account_id{text_in(*said_room, "account")};
    room.title = text_in(*said_room, "title");
    room.kind = conversation_kind_of(text_in(*said_room, "kind"));
    room.last_activity = number_in(*said_room, "last_activity");
    room.unread = static_cast<std::uint32_t>(number_in(*said_room, "unread"));
    room.pinned = truth_in(*said_room, "pinned");
    room.muted = truth_in(*said_room, "muted");
    if (const json::value *models = json::said(*said_room, "models"))
      for (std::size_t at = 0;; ++at) {
        const json::value *model = json::item_of(*models, at);
        if (model == nullptr)
          break;
        if (const auto said_model = json::text_of(*model))
          room.models.push_back(std::string(*said_model));
      }
    room.told = text_in(*said_room, "told");
    room.tokens = static_cast<std::uint32_t>(number_in(*said_room, "tokens"));
    room.window = static_cast<std::uint32_t>(number_in(*said_room, "window"));
    room.context_used = static_cast<float>(decimal_in(*said_room, "context_used"));

    if (const json::value *messages = json::said(*said_room, "messages"))
      for (std::size_t at = 0;; ++at) {
        const json::value *item = json::item_of(*messages, at);
        if (item == nullptr)
          break;
        message one;
        one.id = message_id{text_in(*item, "id")};
        one.chat = room.id;
        one.author = text_in(*item, "author");
        one.outgoing = truth_in(*item, "outgoing");
        one.text = text_in(*item, "text");
        one.sent = number_in(*item, "sent");
        one.state = delivery_of(text_in(*item, "state"));
        one.tokens = static_cast<std::uint32_t>(number_in(*item, "tokens"));
        if (const std::string replies = text_in(*item, "replies_to");
            !replies.empty())
          one.replies_to = message_id{replies};
        if (const json::value *thought_json = json::said(*item, "thought")) {
          thought it;
          it.took_ms = number_in(*thought_json, "took_ms");
          it.tool_calls =
              static_cast<std::uint32_t>(number_in(*thought_json, "tool_calls"));
          it.said = text_in(*thought_json, "said");
          if (const json::value *calls = json::said(*thought_json, "calls"))
            for (std::size_t call_at = 0;; ++call_at) {
              const json::value *call = json::item_of(*calls, call_at);
              if (call == nullptr)
                break;
              it.calls.push_back(call_made{.tool = text_in(*call, "tool"),
                                           .asked = text_in(*call, "asked"),
                                           .answered = text_in(*call, "answered")});
            }
          one.before = std::move(it);
        }
        out.root.messages.push_back(std::move(one));
      }
    out.root.conversations.push_back(std::move(room));
  }

  // Said again in the order it was said: an archive keeps a conversation to a
  // file, and this puts their moments back into one line -- what is within one
  // conversation keeps its own order, and the moment decides between them.
  std::stable_sort(out.root.messages.begin(), out.root.messages.end(),
                   [](const message &left, const message &right) {
                     return left.sent < right.sent;
                   });

  const stored *settings = nullptr;
  for (const stored &one : files)
    if (one.name == "settings.json") {
      settings = &one;
      break;
    }
  if (settings == nullptr) {
    out.trouble = "the archive holds no settings";
    return out;
  }
  const auto said_settings = json::read(settings->body);
  if (!said_settings) {
    out.trouble = "the settings are not written in JSON";
    return out;
  }
  if (const json::value *shown = json::said(*said_settings, "looks")) {
    out.shown.preset = preset_of(text_in(*shown, "preset"));
    out.shown.style = style_of(text_in(*shown, "style"));
    out.shown.mode = mode_of(text_in(*shown, "mode"));
    out.shown.amoled = truth_in(*shown, "amoled");
    out.shown.reduce_motion = truth_in(*shown, "reduce_motion");
    out.shown.avatars = truth_in(*shown, "avatars");
    out.shown.row_times = truth_in(*shown, "row_times");
    out.shown.font_scale =
        static_cast<float>(decimal_in(*shown, "font_scale", 1.0));
    out.shown.font = font_of(text_in(*shown, "font"));
  }
  if (const json::value *how = json::said(*said_settings, "asking")) {
    out.how.thinking = truth_in(*how, "thinking", true);
    out.how.how_hard = effort_of(text_in(*how, "how_hard"));
    out.how.temperature = decimal_in(*how, "temperature", 0.7);
    out.how.top_p = decimal_in(*how, "top_p", 1.0);
    out.how.max_tokens = static_cast<std::uint32_t>(
        number_in(*how, "max_tokens", 2048));
    out.how.presence = decimal_in(*how, "presence");
    out.how.often = decimal_in(*how, "often");
    if (const json::value *tools = json::said(*how, "tools"))
      for (std::size_t at = 0; at < tools_owned; ++at)
        out.how.tools[at] = truth_in(*tools, tool_names[at]);
    if (const json::value *switches = json::said(*how, "switches"))
      for (std::size_t at = 0; at < switches_owned; ++at)
        out.how.switches[at] = truth_in(*switches, switch_names[at]);
  }
  if (const json::value *where = json::said(*said_settings, "door")) {
    // The door by what the settings kept it as: one this program offers, and
    // this page's own server where what is written is none of them -- which
    // is also where a word from an older build lands, so that what is read
    // back is always a door that stands here.
    out.where.provider =
        std::string(provider_by(text_in(*where, "provider")).id);
    // And what opens each door it kept a key for: whole as each was written,
    // under the door it opens -- a key for a door nobody offers anymore
    // stands aside as one for this page's server, and opens nothing until a
    // door of that name is chosen again.
    if (const json::value *keys = json::said(*where, "keys"))
      for (std::size_t at = 0u;; ++at) {
        const json::value *one = json::item_of(*keys, at);
        if (one == nullptr)
          break;
        const std::string id = text_in(*one, "provider");
        if (id.empty())
          continue;
        out.where.keys.push_back(
            door_key{.provider = std::string(provider_by(id).id),
                     .key = text_in(*one, "key")});
      }
  }
  return out;
}

}  // namespace pchat::prgrssv
