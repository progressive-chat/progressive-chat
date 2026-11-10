// SPDX-License-Identifier: AGPL-3.0-only
// pchat.config -- What the program keeps between runs: how it looks, what it
// remembers, and the scheme those looks ask for.
export module pchat.config;

import std;
import pchat.theme;

export namespace pchat {

// Light, dark, or whichever the machine is in.
enum class theme_mode { light, dark, system };

// The faces offered: the one that ships with the program, the system's, or a
// file of the reader's own.
enum class font_choice { app, system, custom };

// How the window looks. One seed, one set of switches, and every colour a
// screen draws comes from them.
struct looks {
  scheme_preset preset = default_preset;
  scheme_style style = default_style;
  theme_mode mode = theme_mode::dark;
  bool amoled = false;
  bool reduce_motion = false;
  // What a row in the drawer is asked for, and is asked for nowhere by
  // default: a circle with the conversation's first letter in it, in the
  // row's own colours, and how long ago what was said last was said.
  bool avatars = false;
  bool row_times = false;
  double font_scale = 1.0;
  font_choice font = font_choice::app;
  friend bool operator==(const looks&, const looks&) = default;
};

// What the mode means, once whether it is dark out is known.
constexpr bool dark_of(theme_mode mode, bool system_dark) {
  switch (mode) {
    case theme_mode::light: return false;
    case theme_mode::dark: return true;
    case theme_mode::system: break;
  }
  return system_dark;
}

// The colours this set of looks is drawn with.
constexpr color_scheme colors_of(const looks& shown, bool system_dark = true) {
  const bool dark = dark_of(shown.mode, system_dark);
  return with_amoled(scheme_for(shown.preset, shown.style, dark), dark, shown.amoled);
}

// The tools this program owns itself (pchat.tools), said by which is which
// because a row in the settings and a body written for a wire both have to
// name the same one.
enum class tool_choice { conversation_search, how_much, when_it_is };
inline constexpr std::size_t tools_owned = 3u;

// Seven things a switch is offered for, said by which is which because a
// row in the settings and what the program keeps have to name the same one:
// two that a picture is asked about, two that are how this program reaches
// other programs, and three that it keeps afterwards.
enum class switch_choice { picture_to_words, words_to_picture, proxy,
                           own_page, remember, skills, data_held,
                           auto_compact };
inline constexpr std::size_t switches_owned = 8u;

// How an answer is asked for: what a model is told to do with its own choices
// while it writes one, whichever wire it is asked through. Five of them, and
// none of them anything this program answers itself -- every one is said as
// part of every body written for a provider (pchat.provider, `wanted`).
// How hard a model is to think before it answers, in the only four ways any
// wire is asked for it: a little, some, most of what it can, or all of it.
// Said once here and written as each family names it by pchat.provider.
enum class effort { least, some, most, all };

// The next of the four, whichever of them stands now: what one press behind
// the card's three dots asks for, and the least of them again after the most.
[[nodiscard]] constexpr effort next_of(effort one) {
  switch (one) {
  case effort::least:
    return effort::some;
  case effort::some:
    return effort::most;
  case effort::most:
    return effort::all;
  case effort::all:
    break;
  }
  return effort::least;
}

[[nodiscard]] inline std::string_view said_by(effort one) {
  switch (one) {
  case effort::least:
    return "low";
  case effort::some:
    return "medium";
  case effort::most:
    return "high";
  case effort::all:
    break;
  }
  return "all";
}

struct asking {
  // That it thinks at all before it answers: off, and a model answers out of
  // whatever it knows; on, and every wire this program speaks to is asked for
  // the thinking in the words that family takes it in.
  bool thinking = true;
  // How hard, where thinking is on at all.
  effort how_hard = effort::some;
  // How far it may wander from the words most likely to come next.
  double temperature = 0.7;
  // How much of what is likely it is allowed to choose between: a share of
  // the whole of it, from all of it down to the likeliest word alone.
  double top_p = 1.0;
  // How long an answer may ever be, in tokens: a ceiling said to the wire,
  // and nothing here counts tokens to keep it.
  std::uint32_t max_tokens = 2048;
  // What is taken off a word for having been said already at all, and for
  // having been said often: the two ways a wire is asked not to repeat.
  double presence = 0.0;
  double often = 0.0;
  // Which of the tools this program owns may be asked for at all: one for
  // each of the three, and every one of them offered until a row says
  // otherwise.
  std::array<bool, tools_owned> tools{true, true, true};
  // Whether each of the seven is offered at all: every one of them on but
  // the two that put this program where other programs can reach it, which
  // are asked for before they are given.
  // Last one on as Agora has it on (docs/en/context.md: automatic compact
  // is enabled by default) -- and off in a file written before it stood,
  // which is what a switch with no word for it reads back as, same as any
  // other that was added after a reader had its say.
  std::array<bool, switches_owned> switches{true,  true, false, false,
                                            true,  true, true,  true};
  friend bool operator==(const asking&, const asking&) = default;
};

inline constexpr asking default_asking{};

// Which of the five one thing is, wherever one of them is named: in what a
// row asks for, and in what the program keeps inside its lines.
enum class asking_what { temperature, top_p, max_tokens, presence, often };

// How far each of them goes, and by how much one press moves it: said once
// here because the row that moves one of them and the program that keeps it
// ask for the same numbers, and a row told its own would soon say another's.
struct asking_range {
  double least = 0.0;
  double most = 1.0;
  double step = 0.1;
};
[[nodiscard]] constexpr asking_range range_of(asking_what one) {
  switch (one) {
  case asking_what::temperature:
    return {.least = 0.0, .most = 2.0, .step = 0.05};
  case asking_what::top_p:
    return {.least = 0.0, .most = 1.0, .step = 0.05};
  case asking_what::max_tokens:
    return {.least = 256.0, .most = 32768.0, .step = 256.0};
  case asking_what::presence:
    return {.least = -2.0, .most = 2.0, .step = 0.1};
  case asking_what::often:
    break;
  }
  return {.least = -2.0, .most = 2.0, .step = 0.1};
}

// One of them kept inside its lines: what a row asked for, whether it asked
// by one press or by being set.
[[nodiscard]] constexpr double kept_asking(asking_what one, double value) {
  const asking_range there = range_of(one);
  return std::clamp(value, there.least, there.most);
}

// What one of them is set to, whichever it is: said as a number everywhere
// but where it is kept as a count of tokens.
[[nodiscard]] constexpr double value_of(const asking &one, asking_what field) {
  switch (field) {
  case asking_what::temperature:
    return one.temperature;
  case asking_what::top_p:
    return one.top_p;
  case asking_what::max_tokens:
    return static_cast<double>(one.max_tokens);
  case asking_what::presence:
    return one.presence;
  case asking_what::often:
    break;
  }
  return one.often;
}

// Whether one of the tools is offered to a model, and the same with that one
// switched: what a row of the Tools group asks for.
[[nodiscard]] constexpr bool offered_of(const asking &one, tool_choice which) {
  return one.tools[static_cast<std::size_t>(which)];
}
[[nodiscard]] constexpr bool offered_of(const asking &one,
                                       switch_choice which) {
  return one.switches[static_cast<std::size_t>(which)];
}
[[nodiscard]] constexpr asking set_of(const asking &one, switch_choice which,
                                      bool on) {
  asking out = one;
  out.switches[static_cast<std::size_t>(which)] = on;
  return out;
}
[[nodiscard]] constexpr asking set_of(const asking &one, tool_choice which,
                                      bool on) {
  asking out = one;
  out.tools[static_cast<std::size_t>(which)] = on;
  return out;
}

// The same, with one of them moved: what a row's press either side of one
// asks for, kept inside the lines that one has.
[[nodiscard]] constexpr asking set_of(const asking &one, asking_what field,
                                      double value) {
  asking out = one;
  switch (field) {
  case asking_what::temperature:
    out.temperature = kept_asking(field, value);
    break;
  case asking_what::top_p:
    out.top_p = kept_asking(field, value);
    break;
  case asking_what::max_tokens:
    out.max_tokens = static_cast<std::uint32_t>(kept_asking(field, value));
    break;
  case asking_what::presence:
    out.presence = kept_asking(field, value);
    break;
  case asking_what::often:
    out.often = kept_asking(field, value);
    break;
  }
  return out;
}

// What is kept: the looks, and the small things a reader would miss.
struct settings_root {
  looks shown;
  asking asked;
  std::vector<std::string> recent_emoji;
  friend bool operator==(const settings_root&, const settings_root&) = default;
};

inline constexpr looks default_looks{};

// The answerers offered, and the room each one's window has: named here
// because which ones there are to answer with is not a screen's to decide,
// and because adding one to a conversation has to add one that answers.
struct answerer_choice {
  // What a wire is given, which is not what it is called: `deepseek-v4-1-flash`
  // is what a body says, and `DeepSeek V4.1 Flash` is what a reader reads.
  std::string_view name;
  // What it is called where it is named on the screen.
  std::string_view said;
  // Who it is asked through: said after its name, in brackets, because two of
  // them may be called alike and the provider is how they are told apart.
  std::string_view alias;
  std::uint32_t window = 0;
};
inline constexpr std::array<answerer_choice, 7> answerers_offered{{
    {"haiku-4", "Claude Haiku 4", "anthropic", 1'000'000},
    {"sonnet-4", "Claude Sonnet 4", "anthropic", 1'000'000},
    {"opus-4", "Claude Opus 4", "anthropic", 1'000'000},
    {"o4-mini", "o4 Mini", "openai", 200'000},
    {"local-8b", "Local 8B", "local", 32'000},
    {"local-70b", "Local 70B", "local", 128'000},
    {"gemini-2.5-flash", "Gemini2.5 Flash", "gemini", 1'048'576},
}};

// What one of them is called on the screen: the model as it is named, and the
// provider it is asked through after it in brackets -- `Claude Haiku 4
// (anthropic)`. One that is offered but nameless says the id it is asked
// through, whole, so that nothing is said of it that is not known.
[[nodiscard]] inline std::string said_as_a_model(std::string_view name) {
  for (const answerer_choice &one : answerers_offered)
    if (one.name == name)
      return std::string(one.said) + " (" + std::string(one.alias) + ")";
  return std::string(name);
}

// Who one of them is asked through, by its name: the family that says which
// wire's words a body and a stream are written in. Empty where nothing knows
// the name, which for a model offered here never happens.
[[nodiscard]] inline std::string_view family_of(std::string_view name) {
  for (const answerer_choice &one : answerers_offered)
    if (one.name == name)
      return one.alias;
  return std::string_view{};
}

// The doors a model may be asked through: Agora's own ten, named here the
// way Agora names them (its port keeps the list, reserved_provider_names,
// so that a reader coming from there finds the same words), and before them
// the door this program asked through until any of them stood here -- this
// page's own server, relative to the page and asked for no key of anybody's.
//
// A base is where each provider stands, as each publishes it: three of them
// are also kept in Agora's port for its own purposes (kOpenAiBaseUrl and
// embeddingBaseUrl hold OpenAI's, OpenRouter's and Ollama's), and the rest
// are the providers' own published addresses. The alias says whose words a
// model nobody here has heard of speaks there -- the same three the
// answerers are told by, with openai standing for the shape most of these
// answer in.
struct provider_choice {
  // What it is kept by, in the settings: one word this program says, and
  // what every run after this one reads the door back by.
  std::string_view id;
  // What it is called on the screen.
  std::string_view said;
  // Where it stands, said whole as far as its version: empty only for this
  // page's own server, whose address is relative to the page (screens,
  // `opened_`).
  std::string_view base;
  // Whose words an unknown model speaks there: "anthropic", "gemini", or
  // "openai" for the shape everything else answers in.
  std::string_view alias;
};
inline constexpr std::array<provider_choice, 11> providers_offered{{
    {"page", "This page's server", "", ""},
    {"openai", "OpenAI", "https://api.openai.com/v1", "openai"},
    {"anthropic", "Anthropic", "https://api.anthropic.com/v1", "anthropic"},
    {"google", "Google", "https://generativelanguage.googleapis.com/v1beta",
     "gemini"},
    {"deepseek", "DeepSeek", "https://api.deepseek.com/v1", "openai"},
    {"qwen", "Qwen", "https://dashscope.aliyuncs.com/compatible-mode/v1",
     "openai"},
    {"groq", "Groq", "https://api.groq.com/openai/v1", "openai"},
    {"openrouter", "Open Router", "https://openrouter.ai/api/v1", "openai"},
    {"opencode", "OpenCode Go", "https://opencode.ai/zen/go/v1", "openai"},
    {"ollama", "Ollama", "http://localhost:11434/v1", "openai"},
    {"local", "Local", "http://localhost:8080/v1", "openai"},
}};

// Which one of them is asked through, by what the settings kept it as: the
// one that says so, and this page's own server where nothing does -- which
// is also where a word from a run of an older build lands, so that a door
// nobody offers anymore is asked of as the door this program falls back to
// and not of nobody at all.
[[nodiscard]] inline const provider_choice &provider_by(
    std::string_view id) {
  for (const provider_choice &one : providers_offered)
    if (one.id == id)
      return one;
  return providers_offered[0];
}

// One door as the reader keeps it: what opens it, and what it may answer
// as -- one entry per door ever used, so that choosing another door keeps
// this one whole. An entry left with nothing in it is not kept at all: a
// door that opens nothing and asks for nothing of its own (this page's
// server, a model run at home) stands with no entry, as it did before any
// entry was here.
struct door_hold {
  std::string provider;
  std::string key;
  // What this door answers as: ids named as their providers name them,
  // offered in a chat beside the answerers this program knows by name --
  // and asked of this door whole, with no lookup anywhere of what a name is
  // called: a model nobody here has heard of gets the room every answer has
  // been given so far (window_of, below).
  std::vector<std::string> models;
};

// Which door the next asking is spoken through, and what is kept of every
// door ever used: with the rest of the settings, so that a run after this
// one asks through the same door, with the same key and the same models
// standing at it, as this one did.
struct door {
  std::string provider = std::string(providers_offered[0].id);
  std::vector<door_hold> holds;
};

// Which key opens one of them: nothing where none was ever typed for it.
[[nodiscard]] inline std::string key_of(const door &where,
                                        std::string_view id) {
  for (const door_hold &one : where.holds)
    if (one.provider == id)
      return one.key;
  return std::string{};
}

// What one of them may answer as: nothing where nothing was ever written
// for it.
[[nodiscard]] inline std::vector<std::string> models_of(const door &where,
                                                        std::string_view id) {
  for (const door_hold &one : where.holds)
    if (one.provider == id)
      return one.models;
  return std::vector<std::string>{};
}

// What stands empty and is not kept as a door at all: an entry goes away
// only once both halves of it are gone -- what opens it, and what it
// answers as.
[[nodiscard]] inline bool holds_nothing(const door_hold &one) {
  return one.key.empty() && one.models.empty();
}

// The key of one door put where it stands: typed over, it goes over the old
// one; typed empty, it goes where nothing else of that door stands either --
// and not before, because the models written for the same door are in that
// entry still.
inline void key_into(door &where, std::string_view id, std::string key) {
  const auto at = std::ranges::find(where.holds, id, &door_hold::provider);
  if (at == where.holds.end()) {
    if (key.empty())
      return;
    where.holds.push_back(
        door_hold{.provider = std::string(id), .key = std::move(key)});
  } else {
    at->key = std::move(key);
  }
  std::erase_if(where.holds, holds_nothing);
}

// What one door may answer as, written whole the way the field says it: its
// entry keeps it, and an entry left with neither a key nor models goes where
// an entry with nothing in it goes.
inline void models_into(door &where, std::string_view id,
                        std::vector<std::string> models) {
  const auto at = std::ranges::find(where.holds, id, &door_hold::provider);
  if (at == where.holds.end()) {
    if (models.empty())
      return;
    where.holds.push_back(door_hold{.provider = std::string(id),
                                    .models = std::move(models)});
  } else {
    at->models = std::move(models);
  }
  std::erase_if(where.holds, holds_nothing);
}

// What a door may answer as, as the field says it: ids apart, the way a
// reader writes them -- and read back the same way. Nothing between them is
// kept: what stands whole between two separators is one id, an id being a
// name and not a phrase, and the same id written twice is asked for once.
[[nodiscard]] inline std::string said_models(
    const std::vector<std::string> &models) {
  std::string out;
  for (const std::string &one : models) {
    if (!out.empty())
      out += ", ";
    out += one;
  }
  return out;
}
[[nodiscard]] inline std::vector<std::string> models_said(
    std::string_view said) {
  std::vector<std::string> out;
  std::size_t from = 0u;
  while (true) {
    const std::size_t comma = said.find(',', from);
    const std::string_view piece = said.substr(
        from, comma == std::string_view::npos ? said.size() - from
                                              : comma - from);
    const std::size_t first = piece.find_first_not_of(' ');
    if (first != std::string_view::npos) {
      const std::size_t last = piece.find_last_not_of(' ');
      std::string id(piece.substr(first, last - first + 1u));
      if (std::ranges::find(out, id) == out.end())
        out.push_back(std::move(id));
    }
    if (comma == std::string_view::npos)
      break;
    from = comma + 1u;
  }
  return out;
}

// Whether one of them answers in a chat, by its name: a chat keeps the ones
// that answer in it, and nothing else says of it that it does.
[[nodiscard]] inline bool answers_in(std::span<const std::string> models,
                                     std::string_view name) {
  return std::ranges::find(models, name) != models.end();
}

// One of them put into a chat, or taken out of it: put in at the end of the
// ones already answering, taken out from wherever it stood, and never the
// last one of them -- a chat with nothing to answer in it is not a thing any
// page can say, so taking out is left undone where it would leave none.
[[nodiscard]] inline std::vector<std::string>
models_with(std::span<const std::string> models, std::string_view name,
            bool wanted) {
  std::vector<std::string> out(models.begin(), models.end());
  const auto at = std::ranges::find(out, name);
  if (wanted) {
    if (at == out.end())
      out.emplace_back(name);
    return out;
  }
  if (at == out.end() || out.size() == 1u)
    return out;
  out.erase(at);
  return out;
}

// How much room an answerer's window has, said by its name: what is offered
// keeps its own, and one that is not offered yet says the room every answer
// has been given so far.
[[nodiscard]] constexpr std::uint32_t window_of(std::string_view name) {
  for (const answerer_choice &one : answerers_offered)
    if (one.name == name)
      return one.window;
  return 1'000'000;
}

}  // namespace pchat
