// SPDX-License-Identifier: AGPL-3.0-only
// pchat.provider -- What one conversation asks of a model, put into words a
// provider's own wire answers to: the turns, the body, the place it is sent.
//
// Nothing here opens anything. It writes the bytes that would go out and reads
// the bytes that would come back (see pchat.provider.stream); what carries
// them is the platform's to do, later, and a test's -- so every part of it can
// be said without a network, which is the only way a protocol is worth
// writing at all.
module;
#include "provider/anthropic/wire.h"
#include "provider/budget.h"

export module pchat.provider;

import std;
import pchat.core;
import pchat.config;
import pchat.json;

export namespace pchat::provider {

// ---- what is said --------------------------------------------------------

// Who said a turn, in the only four ways a wire allows it.
enum class role { system, user, assistant, tool };

[[nodiscard]] inline std::string_view role_text(role one) {
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

// One thing said: by whom, and what. Where thinking came before an answer it
// is kept here too -- said back to nobody yet, because the wires that take it
// back are few and the ones that refuse it are many.
// A tool the model asked for on its way: gathered out of deltas that arrive
// one key at a time, some of which name it and some of which are a piece of
// one argument.
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
  // The tools it stopped to ask for, where it stopped to ask for any: they
  // are given back with the asking, because a wire answers an answer of a
  // call against the call and nothing else.
  std::vector<tool_call> calls;
  // Which call this turn answers, where it answers one: named as the wire
  // named it, which is how what a tool said is read against what asked it.
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
  std::string base;   // as far as its version: https://api.openai.com/v1
  std::string key;
  std::string model;
  std::string told;   // what the model is told about itself
  // Whose words are spoken at this base, said by the door itself (pchat.config,
  // providers_offered): consulted only where no answerer offered here knows
  // the model by its name, because who a model is speaks louder than which
  // door it was asked through. Empty where the door says nothing of it, as
  // where no door stood here yet.
  std::string family;
};

// Two wires wear OpenAI's shape: the chat one everyone answers and the
// responses one only some do. Which is which is said by how the base ends,
// the same way their own docs put it.
// Two wires wear OpenAI's shape: the chat one everyone answers and the
// responses one only some do -- which of those two is said by how the base
// ends, the same way their own docs put it. A third shape is Anthropic's own,
// and no base ending says where that one is asked: the family does, named by
// the answerer the model is.
enum class wire { chat, responses, anthropic, gemini };

[[nodiscard]] inline wire wire_of(const endpoint &one) {
  // Who is asked speaks before the base's ending does: the answerer the
  // model is known by keeps its own wire wherever its base stands, suffixes
  // and all -- Anthropic's family above all, which no base ending says.
  // Where the model is one nobody here has heard of, the door says whose
  // words it speaks (endpoint's `family`, said by the door it stands for);
  // where the door says nothing of them either, only then the base's ending
  // has the word -- a base ending in /v1beta is OpenAI's words under a
  // Google name, and which words the model reads is asked of the model and
  // not of the base's suffix.
  std::string_view family = family_of(one.model);
  if (family.empty())
    family = one.family;
  if (family == "anthropic")
    return wire::anthropic;
  if (family == "gemini")
    return wire::gemini;
  const std::string_view base(one.base);
  if (base.ends_with("/v1beta") || base.ends_with("/responses"))
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
  // Gemini's method carries the model inside it and asks for the answer as
  // the stream of events it is made of: base, model, method, and the form
  // that says the events are the stream and not one blob.
  if (wire_of(one) == wire::gemini) {
    std::string out = one.base;
    out += "/models/";
    out += one.model;
    out += ":streamGenerateContent?alt=sse";
    return out;
  }
  std::string out = one.base;
  out += path_of(wire_of(one));
  return out;
}

// What goes with the body, and nothing that is anybody's secret said twice:
// the key once, in the header a wire asks for it in.
[[nodiscard]] inline std::vector<std::pair<std::string, std::string>>
headers_of(const endpoint &one) {
  std::vector<std::pair<std::string, std::string>> out{
      {"Content-Type", "application/json"}};
  if (wire_of(one) == wire::anthropic) {
    // Where its own wire asks for the key, and its own date beside it: a wire
    // that has a version says so, and this one does.
    if (!one.key.empty())
      out.emplace_back("x-api-key", one.key);
    out.emplace_back("anthropic-version", "2023-06-01");
    return out;
  }
  if (wire_of(one) == wire::gemini) {
    // Where Google's own wire asks for the key: its header's own name and
    // nothing beside it -- which version this wire is has gone into the base
    // of its URL with its method.
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
// the reader wrote asked, and what the model answered, in the order it was
// said and nothing of what the screens know about either. Any range that
// holds messages is given -- a conversation's own vector, a piece of it, a
// view over one -- and not one container of one type.
//
// Where an answer had stopped to ask for something on the way, its round goes
// back the way it happened: the model's asking with the calls in it, what each
// call answered, and only then the answer as it stood. The ids a wire named
// those calls with were read out of the stream and never written down -- what
// was kept is what was asked and what came back -- so the ids are made here,
// one per call of this answer, and both ends of the round are given the same
// one. The words an asking was recorded in are given to a wire as what a wire
// takes: an object, or no arguments at all where it was recorded as plain
// words (call_arguments, src/provider/budget.h, says the same for counting).
template <std::ranges::forward_range Said>
  requires std::same_as<std::ranges::range_value_t<Said>, message>
[[nodiscard]] std::vector<turn> turns_of(Said &&said) {
  std::vector<turn> out;
  for (const message &one : said) {
    // A compact capsule goes out as what it is: what one stretch of this
    // chat hands to whoever answers in it next -- read where a wire keeps
    // what is told, and not as words either side said aloud.
    if (one.capsule) {
      out.push_back(turn{.who = role::system, .text = one.text});
      continue;
    }
    if (one.outgoing) {
      out.push_back(turn{.who = role::user, .text = one.text});
      continue;
    }
    const std::string thought_text =
        one.before ? one.before->said : std::string();
    if (!one.before || one.before->calls.empty()) {
      out.push_back(turn{.who = role::assistant,
                         .text = one.text,
                         .thought = thought_text});
      continue;
    }
    // The asking itself, with nothing of the words before it: those are in the
    // answer as a whole, which comes after the round, where every wire reads
    // them. What it asked for is said here as it was said then.
    turn asking{.who = role::assistant};
    asking.calls.reserve(one.before->calls.size());
    std::vector<turn> answered;
    answered.reserve(one.before->calls.size());
    std::size_t nth = 0;
    for (const call_made &call : one.before->calls) {
      const std::string own =
          "call-" + one.id.value + "-" + std::to_string(nth++);
      asking.calls.push_back(
          tool_call{.id = own,
                    .name = call.tool,
                    .arguments = call_arguments(call.asked)});
      answered.push_back(
          turn{.who = role::tool, .text = call.answered, .own = own});
    }
    out.push_back(std::move(asking));
    for (turn &one_answer : answered)
      out.push_back(std::move(one_answer));
    out.push_back(turn{.who = role::assistant,
                       .text = one.text,
                       .thought = thought_text});
  }
  return out;
}

[[nodiscard]] inline turn system_turn(std::string_view said) {
  return turn{.who = role::system, .text = std::string(said)};
}

// What a model may be told it can ask for on its way to answering: named,
// told what it does, and told what each thing it takes is -- all of it in the
// shape a wire reads it back in, and none of it anything this tree runs.
struct offered {
  std::string name;
  std::string told;
  // What it takes, each of them named and said about, and every one of them
  // a string: nothing here asks a model to write JSON that is more than text.
  std::vector<std::pair<std::string, std::string>> taken;
};

// What a body is written with besides the turns: that it streams, what tools
// it may ask for on its way, and how it is to choose every word it writes.
//
// Every one of the five is kept as it was asked for and said only where it was
// asked for: a wire keeps its own for each, and a program that has not been
// told one has nothing to say about it.
struct wanted {
  bool stream = true;
  // That it thinks before it answers, and how hard: written as the family
  // being spoken to names it, because no two of them do -- openai's own wire
  // takes a reasoning effort, the relays that carry many take a thing of that
  // name inside one, and deepseek takes nothing at all, thinking whenever it
  // answers and never asked to.
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
// keeps them and into what a body is written with. Nothing of the two is
// lost going either way, and nothing is said for one that was never set.
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

// What an anthropic wire wants said to it: the same things in its own words --
// the turns as content blocks, what it is told as a field named `system`, and
// the choices written in the order its own writer writes them (port
// anthropic_request.cpp, write_request_body). What this shape has no field for
// is not said: thinking is asked for in the budget this family takes it in,
// where the model takes a thinking at all, and the penalties a chat wire takes
// are not Anthropic's.
template <std::ranges::forward_range Turns>
  requires std::same_as<std::ranges::range_value_t<Turns>, turn>
[[nodiscard]] inline std::string anthropic_body_of(const endpoint &one, Turns &&turns,
                                                   const wanted &wants = {}) {
  json::writing out;
  out.thing();
  out.name("model").text(one.model);
  out.name("messages").list();
  // What it is told, and what every turn said as system adds to it: one place
  // a wire keeps those words in, and the ones the reader wrote for this
  // conversation come in through it too (as the chat wire takes them).
  std::string system(one.told);
  for (const turn &said : turns) {
    if (said.who == role::system) {
      if (!system.empty())
        system += "\n\n";
      system += said.text;
      continue;
    }
    out.thing().name("role").text(role_text(said.who)).name("content").list();
    if (said.who == role::tool && !said.own.empty()) {
      // One answered call, as the wire reads it back: against the call it
      // answers, and nothing else in this message.
      out.thing()
          .name("type").text("tool_result")
          .name("tool_use_id").text(said.own)
          .name("content").text(said.text)
          .shut();
    } else if (!said.calls.empty()) {
      // What it answered, then every call it stopped to ask for, each block
      // its own: the wire keeps text and tool_use side by side.
      if (!said.text.empty())
        out.thing().name("type").text("text").name("text").text(said.text).shut();
      for (const tool_call &call : said.calls)
        out.thing()
            .name("type").text("tool_use")
            .name("id").text(call.id)
            .name("name").text(call.name)
            .name("input").raw(call.arguments.empty() ? "{}" : call.arguments)
            .shut();
    } else {
      out.thing().name("type").text("text").name("text").text(said.text).shut();
    }
    out.shut();
    out.shut();
  }
  out.shut();
  if (!system.empty())
    out.name("system").text(system);
  // The ceiling the model family allows, and the asking's own count under it:
  // Anthropic reads max_tokens on every body, so one is always said.
  const long long ceiling = pchat_anthropic_ceiling(one.model.c_str());
  // Whether this model thinks before it answers, and what it is given to
  // think with: the reader asking for it, and this model being able to take
  // what is asked of it. Nothing of it is said below where either is not
  // there, which is how a wire is told no thinking without a word about it.
  const anthropic_thinking_cap cap = pchat_anthropic_thinking(one.model.c_str());
  const bool thinks = wants.thinking && cap.thinking[0] != '\0';
  // Thinking is written out of the same room the answer is written in, so
  // where it is asked for, the room is made to hold it first: the budget, and
  // the headroom to answer in beside it, as Agora makes room before it asks
  // (AnthropicProvider, ANSWER_HEADROOM_TOKENS), under the same ceiling.
  constexpr std::int64_t headroom = 8192;
  std::int64_t room =
      wants.max_tokens ? std::min<std::int64_t>(*wants.max_tokens, ceiling) : ceiling;
  if (thinks)
    room = std::min<std::int64_t>(std::max<std::int64_t>(room, cap.budget + headroom),
                                  ceiling);
  out.name("max_tokens").number(room);
  out.name("stream").truth(wants.stream);
  if (thinks) {
    out.name("thinking").thing().name("type").text(cap.thinking);
    if (cap.budget > 0)
      out.name("budget_tokens").number(static_cast<std::int64_t>(cap.budget));
    out.name("display").text("summarized").shut();
  }
  // The two numbers that choose its words, read where this model still reads
  // them: sampling is taken off the generations that think (temperature not at
  // all beside a thinking, and top_p only in the 0.95..1 that is left legal
  // there), which is the condition Agora keeps on the same two fields.
  if (cap.sampling && !thinks && wants.temperature)
    out.name("temperature").number(*wants.temperature);
  if (cap.sampling && wants.top_p &&
      (!thinks || (*wants.top_p >= 0.95 && *wants.top_p <= 1.0)))
    out.name("top_p").number(*wants.top_p);
  if (!wants.tools.empty()) {
    out.name("tools").list();
    for (const offered &one_tool : wants.tools) {
      out.thing();
      out.name("name").text(one_tool.name);
      out.name("description").text(one_tool.told);
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
// What this shape has no field for is not said: the penalties a chat wire
// takes are not Google's, and thinking is a budget of tokens here -- named
// in the thousands when the reader asked for thinking, and named zero when
// they did not, because this wire thinks on by itself where no budget says
// otherwise and this tree says otherwise.
template <std::ranges::forward_range Turns>
  requires std::same_as<std::ranges::range_value_t<Turns>, turn>
[[nodiscard]] inline std::string gemini_body_of(const endpoint &one, Turns &&turns,
                                                const wanted &wants = {}) {
  json::writing out;
  out.thing();
  out.name("contents").list();
  // What it is told, standing as an instruction of its own beside whatever
  // turns were said as system -- the same words the chat wire puts first.
  std::string system(one.told);
  // What each call went out under, kept as it is answered: a functionResponse
  // answers by the name of the call, and a turn carries only the id of the
  // one it answers.
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
    // model's; an answer to a call goes back as a user's part against the
    // call it answers, which is this wire's own word for it.
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
      out.name("description").text(one_tool.told);
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
// the turns, and that it says what it cost at the end of the stream and not
// only in the answer no stream gives.
template <std::ranges::forward_range Turns>
  requires std::same_as<std::ranges::range_value_t<Turns>, turn>
[[nodiscard]] std::string body_of(const endpoint &one, Turns &&turns,
                                  const wanted &wants = {}) {
  // The first shape is this one; where the family asks for Anthropic's own
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
  if (!one.told.empty() || any_turn) {
    out.name("messages").list();
    // What it is told goes in as the first turn and nowhere else: a wire
    // takes one system turn, and this body is written once for one answer.
    if (!one.told.empty())
      out.thing()
          .name("role").text(role_text(role::system))
          .name("content").text(one.told)
          .shut();
    for (const turn &said : turns) {
      out.thing().name("role").text(role_text(said.who));
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
  // How it is to write, where anything was said about that: five numbers a
  // wire reads whether it streams or not, and none of them written for a wire
  // that was told nothing about how to choose.
  if (wants.thinking && wants.how_hard) {
    const std::string hard = std::string(effort_label(*wants.how_hard));
    const std::string_view base(one.base);
    if (base.find("openrouter") != std::string::npos)
      out.name("reasoning").thing().name("effort").text(hard).shut();
    else if (base.find("deepseek") == std::string::npos)
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
  // What it cost comes at the end of the stream too: the last frame of one has
  // no choice in it, and without this there would be nothing in it to read.
  if (wants.stream)
    out.name("stream_options").thing().name("include_usage").truth(true).shut();
  if (!wants.tools.empty()) {
    out.name("tools").list();
    for (const offered &one_tool : wants.tools) {
      out.thing().name("type").text("function").name("function").thing();
      out.name("name").text(one_tool.name);
      out.name("description").text(one_tool.told);
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
