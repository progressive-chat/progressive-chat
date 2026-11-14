// SPDX-License-Identifier: AGPL-3.0-only
// pchat.provider -- What one conversation asks of a model, in the words a
// provider's wire answers to: the turns, the body, the place it is sent.
//
// Nothing here opens anything. It writes the bytes that would go out and
// reads the bytes that would come back (see pchat.provider.stream). What
// carries them is the platform's job later and a test's, so every part of it
// can be exercised with no network.
module;
#include "provider/anthropic/wire.h"
#include "agora/core/anthropic_request.hpp"
#include "agora/core/json.hpp"

export module pchat.provider;

import std;
import pchat.core;
import pchat.config;
import pchat.json;

export namespace pchat::provider {

// ---- what is said --------------------------------------------------------

// Who said a turn, in the four ways a wire allows.
enum class role { system, user, assistant, tool };

[[nodiscard]] inline std::string_view said_by(role one) {
  switch (one) {
  case role::system:
    return "system";
  case role::user:
    return "user";
  case role::assistant:
    return "assistant";
  case role::tool:
    return "tool";
  }
  return "user";
}

// One thing said: by whom, and what. Thinking that came before an answer is
// kept here too, sent back to nobody for now: few wires take it back and many
// refuse it. A tool the model asked for on its way: gathered out of deltas
// arriving one key at a time, some naming it and some a piece of one
// argument.
struct tool_call {
  std::string id;
  std::string name;
  std::string arguments;
  friend bool operator==(const tool_call &, const tool_call &) = default;
};

struct turn {
  role who = role::user;
  std::string text;
  std::string thought;
  // The tools it stopped to ask for, where it stopped to ask for any: given
  // back with the asking, a wire answering a call against that call and
  // nothing else.
  std::vector<tool_call> calls;
  // Which call this turn answers, where it answers one: named as the wire
  // named it, so what a tool said reads against what asked it.
  std::string own;
  friend bool operator==(const turn &, const turn &) = default;
};

// What an answer cost, as a wire counts it: what went in and what came out.
struct usage {
  std::uint32_t in = 0;
  std::uint32_t out = 0;
  friend bool operator==(const usage &, const usage &) = default;
};

// ---- where it is said ----------------------------------------------------

// A model, and the door it is spoken through: where the wire is, the key that
// opens it, which model answers, and what it is told before anything is asked.
struct endpoint {
  std::string base_url;  // as far as its version: https://api.openai.com/v1
  std::string key;
  std::string model;
  std::string system_prompt;  // what the model is told about itself
};

// Two wires wear OpenAI's shape: the chat one everyone answers and the
// responses one only some do. Which of the two is told by how the base ends,
// the way their own docs put it. A third shape is Anthropic's own, and no
// base ending says where that one is asked: the family does, named by the
// answerer the model is.
enum class wire { chat, responses, anthropic, gemini };

[[nodiscard]] inline wire wire_of(const endpoint &one) {
  // Who is asked speaks before the base's ending: Anthropic's family keeps
  // its own wire wherever its base stands, suffixes and all.
  if (family_of(one.model) == "anthropic")
    return wire::anthropic;
  // The family before the base's ending here too: a base ending in /v1beta is
  // OpenAI's words under a Google name, and which words the model reads is
  // asked of the model, not of the base's suffix.
  if (family_of(one.model) == "gemini")
    return wire::gemini;
  const std::string_view base_url(one.base_url);
  if (base_url.ends_with("/v1beta") || base_url.ends_with("/responses"))
    return wire::responses;
  return wire::chat;
}

[[nodiscard]] inline std::string_view path_of(wire one) {
  switch (one) {
  case wire::chat:
    return "/chat/completions";
  case wire::responses:
    return "/responses";
  case wire::anthropic:
    return "/messages";
  case wire::gemini:
    // The method is the path here, model and all: see url_of.
    return "";
  }
  return "/chat/completions";
}

[[nodiscard]] inline std::string url_of(const endpoint &one) {
  // Gemini's method carries the model inside it and asks for the answer as a
  // stream of events: base, model, method, and the form saying the events are
  // the stream and not one blob.
  if (wire_of(one) == wire::gemini) {
    std::string out = one.base_url;
    out += "/models/";
    out += one.model;
    out += ":streamGenerateContent?alt=sse";
    return out;
  }
  std::string out = one.base_url;
  out += path_of(wire_of(one));
  return out;
}

// What goes with the body, and no secret said twice: the key once, in the
// header a wire asks for it in.
[[nodiscard]] inline std::vector<std::pair<std::string, std::string>>
headers_of(const endpoint &one) {
  std::vector<std::pair<std::string, std::string>> out{
      {"Content-Type", "application/json"}};
  if (wire_of(one) == wire::anthropic) {
    // Where its own wire asks for the key, and its date beside it: this
    // versioned wire says its version.
    if (!one.key.empty())
      out.emplace_back("x-api-key", one.key);
    out.emplace_back("anthropic-version", "2023-06-01");
    return out;
  }
  if (wire_of(one) == wire::gemini) {
    // Where Google's own wire asks for the key: its header's name and nothing
    // beside it. The wire's version went into the URL base with the method.
    if (!one.key.empty())
      out.emplace_back("x-goog-api-key", one.key);
    return out;
  }
  if (!one.key.empty())
    out.emplace_back("Authorization", "Bearer " + one.key);
  return out;
}

// ---- the turns of a conversation -----------------------------------------

// What the model keeps of one conversation, as turns a wire is given: what
// the reader wrote asked and what the model answered, in order, none of what
// the screens know. Any range of messages is accepted -- a conversation's
// vector, a piece of it, a view over one -- not one container type.
template <std::ranges::forward_range Said>
  requires std::same_as<std::ranges::range_value_t<Said>, message>
[[nodiscard]] std::vector<turn> turns_of(Said &&said) {
  return std::ranges::to<std::vector<turn>>(
      said | std::views::transform([](const message &one) {
        return turn{.who = one.outgoing ? role::user : role::assistant,
                    .text = one.text,
                    .thought = one.before ? one.before->said : std::string()};
      }));
}

[[nodiscard]] inline turn told_as_system(std::string_view said) {
  return turn{.who = role::system, .text = std::string(said)};
}

// What a model may be told it can ask for on its way to answering: named,
// told what it does, and told what each thing it takes is, in the shape a
// wire reads it back in. Nothing here runs anything.
struct offered {
  std::string name;
  // What the model is told it does, in the words a tool is documented by.
  std::string description;
  // What it takes, each named and said about, every one a string: no JSON
  // here beyond text.
  std::vector<std::pair<std::string, std::string>> taken;
};

// What a body is written with besides the turns: that it streams, what tools
// it may ask for on its way, and how it chooses every word it writes.
//
// Every one of the five is kept as asked for and said only where it was asked
// for: a wire keeps its own for each, and a program told nothing has nothing
// to say about it.
struct wanted {
  bool stream = true;
  // That it thinks before it answers, and how hard: written as the family
  // spoken to names it, for no two name it alike. openai's wire takes a
  // reasoning effort, relays carrying many take a field of that name inside
  // one, deepseek takes nothing: it thinks whenever it answers.
  bool thinking = false;
  std::optional<effort> how_hard;
  std::vector<offered> tools;
  std::optional<double> temperature;
  std::optional<double> top_p;
  std::optional<std::uint32_t> max_tokens;
  std::optional<double> presence_penalty;
  std::optional<double> frequency_penalty;
};

// ---- the body ------------------------------------------------------------

// How the program asks, in the words a wire takes it in: the five things an
// answer is chosen by (pchat.config, `asking`), read out of where the reader
// keeps them into what a body is written with. Nothing is lost going either
// way, and nothing is said for one never set.
[[nodiscard]] inline wanted wants_of(const asking &how) {
  return wanted{.stream = true,
                .thinking = how.thinking,
                .how_hard = how.thinking ? std::optional<effort>(how.how_hard)
                                         : std::nullopt,
                .temperature = how.temperature,
                .top_p = how.top_p,
                .max_tokens = how.max_tokens,
                .presence_penalty = how.presence,
                .frequency_penalty = how.often};
}

// One turn said in the ported tree's own words, so the ported writer writes it:
// the role the wire reads, and the content parts -- a tool's answer tied to the
// call it answers, the calls a model stopped to ask for beside the text it
// said, or the text alone. The ported tree serializes it; this builds it.
[[nodiscard]] inline agora::core::anthropic::AnthropicMessage as_anthropic(
    const turn &said) {
  using agora::core::anthropic::AnthropicContentPart;
  using agora::core::anthropic::AnthropicMessage;
  AnthropicMessage out;
  out.role = said_by(said.who);
  if (said.who == role::tool && !said.own.empty()) {
    AnthropicContentPart part;
    part.type = "tool_result";
    part.tool_use_id = said.own;
    part.content = said.text;
    out.content.push_back(std::move(part));
  } else if (!said.calls.empty()) {
    if (!said.text.empty()) {
      AnthropicContentPart text;
      text.type = "text";
      text.text = said.text;
      out.content.push_back(std::move(text));
    }
    for (const tool_call &call : said.calls) {
      AnthropicContentPart use;
      use.type = "tool_use";
      use.id = call.id;
      use.name = call.name;
      const std::string input = call.arguments.empty() ? "{}" : call.arguments;
      // The input is the call's own arguments, parsed by the ported reader:
      // the value the ported writer puts back out, not our own read of it.
      const agora::core::json::ParseResult parsed =
          agora::core::json::parse(input);
      if (parsed.value)
        use.input = *parsed.value;
      out.content.push_back(std::move(use));
    }
  } else {
    AnthropicContentPart text;
    text.type = "text";
    text.text = said.text;
    out.content.push_back(std::move(text));
  }
  return out;
}

// What an anthropic wire wants said to it: the same things in its own words
// -- the turns as content blocks, what it is told as a field named `system`,
// and the choices written in the order its own writer writes them (port
// anthropic_request.cpp, write_request_body). Fields this shape lacks are
// omitted: thinking arrives with budgets only where the asking carries them
// through the preparation this tree does not do yet, and the penalties a chat
// wire takes are not Anthropic's.
template <std::ranges::forward_range Turns>
  requires std::same_as<std::ranges::range_value_t<Turns>, turn>
[[nodiscard]] inline std::string anthropic_body_of(const endpoint &one, Turns &&turns,
                                                   const wanted &wants = {}) {
  json::writing out;
  out.thing();
  out.name("model").text(one.model);
  out.name("messages").list();
  std::string system;
  for (const turn &said : turns) {
    if (said.who == role::system) {
      if (!system.empty())
        system += "\n\n";
      system += said.text;
      continue;
    }
    // One message written by the ported tree's own writer: as_anthropic says
    // the turn in the ported shape, and the ported writer sends its bytes
    // into this list as they are. What the wire reads is one serializer's word
    // for it now, not two.
    out.raw(agora::core::anthropic::write_anthropic_message(as_anthropic(said)));
  }
  out.shut();
  if (!system.empty())
    out.name("system").text(system);
  // The ceiling the model family allows, and the asking's own count under it:
  // Anthropic reads max_tokens on every body, so one is always said.
  const long long ceiling = pchat_anthropic_ceiling(one.model.c_str());
  const std::int64_t room =
      wants.max_tokens ? std::min<std::int64_t>(*wants.max_tokens, ceiling) : ceiling;
  out.name("max_tokens").number(room);
  out.name("stream").truth(wants.stream);
  if (wants.temperature)
    out.name("temperature").number(*wants.temperature);
  if (wants.top_p)
    out.name("top_p").number(*wants.top_p);
  if (!wants.tools.empty()) {
    out.name("tools").list();
    for (const offered &one_tool : wants.tools) {
      out.thing();
      out.name("name").text(one_tool.name);
      out.name("description").text(one_tool.description);
      out.name("input_schema").thing().name("type").text("object");
      out.name("properties").thing();
      for (const auto &[takes, said] : one_tool.taken)
        out.name(takes).thing().name("type").text("string").name("description").text(said).shut();
      out.shut();
      out.name("required").list();
      for (const auto &[takes, said] : one_tool.taken)
        out.text(takes);
      out.shut();
      out.shut().shut();
    }
    out.shut();
  }
  out.shut();
  return out.take();
}

// What a gemini wire wants said to it: the same things in its own words --
// the turns as contents of parts, what it is told as a systemInstruction
// beside them, and every choice an answer is made by under one
// generationConfig of this wire's own naming (Gemini's generateContent body).
// Fields this shape lacks are omitted: the penalties a chat wire takes are
// not Google's, and thinking is a token budget here -- in the thousands when
// the reader asked for thinking, zero when not: no budget says otherwise and
// this tree says otherwise.
template <std::ranges::forward_range Turns>
  requires std::same_as<std::ranges::range_value_t<Turns>, turn>
[[nodiscard]] inline std::string gemini_body_of(const endpoint &one, Turns &&turns,
                                                const wanted &wants = {}) {
  json::writing out;
  out.thing();
  out.name("contents").list();
  std::string system;
  // What each call went out under, kept as it is answered: a functionResponse
  // answers by the call's name, and a turn carries only the id of the call it
  // answers.
  std::vector<std::pair<std::string, std::string>> asked;
  for (const turn &said : turns) {
    if (said.who == role::system) {
      if (!system.empty())
        system += "\n\n";
      system += said.text;
      continue;
    }
    for (const tool_call &call : said.calls)
      asked.emplace_back(call.id, call.name);
    // What a person wrote is a user's and what the model answered is a
    // model's. An answer to a call goes back as a user's part against the
    // call it answers, this wire's word for it.
    const std::string_view speaks =
        said.who == role::assistant ? "model" : std::string_view("user");
    out.thing().name("role").text(speaks).name("parts").list();
    if (said.who == role::tool && !said.own.empty()) {
      std::string called = said.own;
      for (const auto &[id, name] : asked)
        if (id == said.own)
          called = name;
      out.thing().name("functionResponse").thing().name("name").text(called);
      out.name("response").thing().name("result").text(said.text).shut();
      out.shut().shut();
    } else if (!said.calls.empty()) {
      if (!said.text.empty())
        out.thing().name("text").text(said.text).shut();
      for (const tool_call &call : said.calls) {
        out.thing().name("functionCall").thing().name("name").text(call.name);
        out.name("args").raw(call.arguments.empty() ? "{}" : call.arguments);
        out.shut().shut();
      }
    } else {
      out.thing().name("text").text(said.text).shut();
    }
    out.shut();
    out.shut();
  }
  out.shut();
  if (!system.empty()) {
    out.name("systemInstruction").thing();
    out.name("parts").list();
    out.thing().name("text").text(system).shut();
    out.shut();
    out.shut();
  }
  out.name("generationConfig").thing();
  if (wants.temperature)
    out.name("temperature").number(*wants.temperature);
  if (wants.top_p)
    out.name("topP").number(*wants.top_p);
  if (wants.max_tokens)
    out.name("maxOutputTokens").number(static_cast<std::int64_t>(*wants.max_tokens));
  out.name("thinkingConfig").thing();
  out.name("thinkingBudget").number(static_cast<std::int64_t>(wants.thinking ? 8192 : 0));
  out.shut();
  out.shut();
  if (!wants.tools.empty()) {
    out.name("tools").list();
    out.thing().name("functionDeclarations").list();
    for (const offered &one_tool : wants.tools) {
      out.thing();
      out.name("name").text(one_tool.name);
      out.name("description").text(one_tool.description);
      out.name("parameters").thing().name("type").text("object");
      out.name("properties").thing();
      for (const auto &[takes, said] : one_tool.taken)
        out.name(takes).thing().name("type").text("string").name("description").text(said).shut();
      out.shut();
      out.name("required").list();
      for (const auto &[takes, said] : one_tool.taken)
        out.text(takes);
      out.shut();
      out.shut().shut();
    }
    out.shut();
    out.shut();
    out.shut();
  }
  out.shut();
  return out.take();
}

// What a chat wire wants said to it: the model, that it is asked to stream,
// the turns, and that it says what it cost at the end of the stream, not only
// in the answer a stream never gives.
template <std::ranges::forward_range Turns>
  requires std::same_as<std::ranges::range_value_t<Turns>, turn>
[[nodiscard]] std::string body_of(const endpoint &one, Turns &&turns,
                                  const wanted &wants = {}) {
  // The first shape is this one. Where the family asks for Anthropic's own
  // words instead, they are what go out.
  if (wire_of(one) == wire::anthropic)
    return anthropic_body_of(one, turns, wants);
  // And where it asks for Google's: the same things again, said the way
  // generateContent reads them.
  if (wire_of(one) == wire::gemini)
    return gemini_body_of(one, turns, wants);
  json::writing out;
  out.thing();
  out.name("model").text(one.model);
  out.name("stream").truth(wants.stream);
  const bool any_turn = std::ranges::begin(turns) != std::ranges::end(turns);
  if (!one.system_prompt.empty() || any_turn) {
    out.name("messages").list();
    // What it is told goes in as the first turn and nowhere else: a wire
    // takes one system turn, and this body is written once for one answer.
    if (!one.system_prompt.empty())
      out.thing()
          .name("role").text(said_by(role::system))
          .name("content").text(one.system_prompt)
          .shut();
    for (const turn &said : turns) {
      out.thing().name("role").text(said_by(said.who));
      if (said.who == role::tool && !said.own.empty())
        out.name("tool_call_id").text(said.own);
      out.name("content").text(said.text);
      if (!said.calls.empty()) {
        out.name("tool_calls").list();
        for (const tool_call &one : said.calls)
          out.thing()
              .name("id").text(one.id)
              .name("type").text("function")
              .name("function").thing()
              .name("name").text(one.name)
              .name("arguments").text(one.arguments)
              .shut()
              .shut();
        out.shut();
      }
      out.shut();
    }
    out.shut();
  }
  // How it writes, where anything was said about that: five numbers a wire
  // reads whether it streams or not, none written for a wire told nothing
  // about how to choose.
  if (wants.thinking && wants.how_hard) {
    const std::string hard = std::string(said_by(*wants.how_hard));
    const std::string_view base_url(one.base_url);
    if (base_url.find("openrouter") != std::string::npos)
      out.name("reasoning").thing().name("effort").text(hard).shut();
    else if (base_url.find("deepseek") == std::string::npos)
      out.name("reasoning_effort").text(hard);
  }
  if (wants.temperature)
    out.name("temperature").number(*wants.temperature);
  if (wants.top_p)
    out.name("top_p").number(*wants.top_p);
  if (wants.max_tokens)
    out.name("max_tokens").number(static_cast<std::int64_t>(*wants.max_tokens));
  if (wants.presence_penalty)
    out.name("presence_penalty").number(*wants.presence_penalty);
  if (wants.frequency_penalty)
    out.name("frequency_penalty").number(*wants.frequency_penalty);
  // What it cost comes at the end of the stream too: the last frame of one
  // has no choice in it, and without this it has nothing to read.
  if (wants.stream)
    out.name("stream_options").thing().name("include_usage").truth(true).shut();
  if (!wants.tools.empty()) {
    out.name("tools").list();
    for (const offered &one_tool : wants.tools) {
      out.thing().name("type").text("function").name("function").thing();
      out.name("name").text(one_tool.name);
      out.name("description").text(one_tool.description);
      out.name("parameters").thing().name("type").text("object");
      out.name("properties").thing();
      for (const auto &[takes, said] : one_tool.taken)
        out.name(takes).thing().name("type").text("string").name("description").text(said).shut();
      out.shut();
      out.name("required").list();
      for (const auto &[takes, said] : one_tool.taken)
        out.text(takes);
      out.shut();
      out.shut().shut();
    }
    out.shut();
  }
  out.shut();
  return out.take();
}

} // namespace pchat::provider
