// SPDX-License-Identifier: AGPL-3.0-only
// pchat.prgrssv -- Everything the program holds, written whole into one file:
// a ZIP archive under the extension .prgrssv, so what was said here can be
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
// to one name, so nothing written here reads back as something else.
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
// keeps, so an archive read back lines up with what the page shows.
inline constexpr std::array<std::string_view, tools_owned> tool_names{
    "conversation_search", "how_much", "when_it_is"};
inline constexpr std::array<std::string_view, switches_owned> switch_names{
    "picture_to_words", "words_to_picture", "proxy",
    "own_page",         "remember",         "skills", "data_held"};

}  // namespace pchat::prgrssv

export namespace pchat::prgrssv {

// What every archive calls itself, and how far it has got: said in the
// manifest, so anything reading one file knows what the rest of them are
// written as.
inline constexpr std::string_view format = "prgrssv";
inline constexpr std::int64_t version = 1;

// One archive, whole: what the program holds said into the files it is kept
// in, and those files zipped into the bytes a save is handed.
[[nodiscard]] inline std::string archive_of(const chats_root &root,
                                            const looks &shown,
                                            const asking &how) {
  std::vector<stored> files;

  // The manifest: what this archive is, whose program wrote it, who its
  // conversations were spoken through, and how much of everything stands in
  // it: the index anything reading the file opens first.
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
  out.shut();
  files.push_back(stored{"manifest.json", out.take()});

  // One file per conversation: the row of the drawer whole, and every message
  // in it in the order it was said: what said each, what thought an answer
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
    written.name("system_prompt").text(room.system_prompt);
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
      if (one.why_failed)
        written.name("why").text(*one.why_failed);
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
  // is asked for, each said whole: what was on the switches and the knobs
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
  settings.shut();  // looks-and-asking
  files.push_back(stored{"settings.json", settings.take()});

  return zipped(files);
}

}  // namespace pchat::prgrssv
