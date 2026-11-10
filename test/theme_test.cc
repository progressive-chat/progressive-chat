module;
#include "tests.h"
export module pchat.test.theme;
// SPDX-License-Identifier: AGPL-3.0-only
// The colours: every preset gives a scheme whose roles are kept, and AMOLED's
// black is blacker than the dark scheme's own.
import std;
import pchat.theme;
import pchat.config;



using namespace pchat;

SAID(theme, every_preset_gives_a_scheme) {
  for (const preset_seed& seed : preset_seeds) {
    const color_scheme light = scheme_for(seed.preset, default_style, false);
    const color_scheme dark = scheme_for(seed.preset, default_style, true);
    EXPECT_TRUE(light != dark);
    EXPECT_FALSE(name_of(seed.preset).empty());
    EXPECT_EQ(seed_of(seed.preset), seed.seed);
  }
  EXPECT_EQ(std::ranges::distance(preset_seeds), 8);
}

SAID(theme, amoled_blacks_out_the_background) {
  const color_scheme dark = scheme_for(default_preset, default_style, true);
  const color_scheme amoled = amoled_scheme(dark, true, true);
  EXPECT_NE(dark.background, 0u);
  EXPECT_EQ(amoled.background & 0x00ffffffu, 0u);
  EXPECT_NE(amoled.primary, 0u);
}

SAID(theme, looks_give_colours_for_the_mode) {
  EXPECT_NE((colors_of(looks{.preset = scheme_preset::midnight, .mode = theme_mode::dark})),
            (colors_of(looks{.preset = scheme_preset::midnight, .mode = theme_mode::light})));
  EXPECT_FALSE(dark_of(theme_mode::light, true));
  EXPECT_TRUE(dark_of(theme_mode::system, true));
}

SAID(theme, what_an_answer_is_asked_with_keeps_inside_its_lines) {
  EXPECT_EQ(value_of(default_asking, asking_what::temperature), 0.7);
  EXPECT_EQ(value_of(default_asking, asking_what::max_tokens), 2048.0);
  // Too hot to stand, and below nothing at all: both kept by `clamped_value`.
  EXPECT_EQ(clamped_value(asking_what::temperature, 9.0), 2.0);
  EXPECT_EQ(clamped_value(asking_what::top_p, -1.0), 0.0);
  // One press either side of one moves it by that one's own step, and stops
  // where it stops.
  asking one = default_asking;
  one = set_of(one, asking_what::temperature, 0.7 + range_of(asking_what::temperature).step);
  EXPECT_EQ(one.temperature, 0.7 + range_of(asking_what::temperature).step);
  one = set_of(set_of(one, asking_what::max_tokens, 0.0), asking_what::max_tokens,
               -4096.0);
  EXPECT_EQ(one.max_tokens, 256u);
  // A press that says one of them says nothing of the others.
  EXPECT_EQ(one.top_p, default_asking.top_p);
  EXPECT_EQ(one.often, default_asking.often);
}

SAID(config, an_answerer_is_put_in_and_taken_out_of_a_chat) {
  std::vector<std::string> models{"haiku-4"};
  EXPECT_FALSE(answers_in(models, "opus-4"));
  EXPECT_TRUE(answers_in(models, "haiku-4"));
  models = models_with(models, "opus-4", true);
  EXPECT_EQ(models.size(), 2u);
  EXPECT_EQ(models.front(), "haiku-4");
  EXPECT_TRUE(answers_in(models, "opus-4"));
  // Put in again, and it is not in it twice.
  models = models_with(models, "opus-4", true);
  EXPECT_EQ(models.size(), 2u);
  // Taken out: the first of them answers from then on, and the room an
  // answer has is that one's own, which every answerer here has.
  models = models_with(models, "haiku-4", false);
  EXPECT_EQ(models.size(), 1u);
  EXPECT_EQ(models.front(), "opus-4");
  EXPECT_EQ(window_of(models.front()), 1'000'000u);
  // And the last of them is never taken out: a chat nothing answers in is
  // not a thing any page can say.
  models = models_with(models, "opus-4", false);
  EXPECT_EQ(models.size(), 1u);
  EXPECT_TRUE(answers_in(models, "opus-4"));
  // Taking out one that was never in it leaves what is there as it was.
  models = models_with(models, "local-8b", false);
  EXPECT_EQ(models.size(), 1u);
  EXPECT_EQ(window_of(models.front()), 1'000'000u);
  // Every one of them offered is one a chat can be given.
  for (const answerer_choice &one : answerers_offered)
    EXPECT_TRUE(answers_in(models_with(models, one.name, true), one.name));
}

SAID(config, every_door_says_where_it_stands_and_whose_words_are_there) {
  // The door this program asked through before any door stood here is the
  // first one, and stands without a base of its own: this page's server is
  // this page's own, said relative to it, and needs no key of anybody's.
  EXPECT_EQ(std::string(providers_offered[0].id), std::string("page"));
  EXPECT_TRUE(providers_offered[0].base.empty());
  EXPECT_TRUE(providers_offered[0].alias.empty());

  // Every id one word of its own, and every id standing alone: no two doors
  // are kept by the same word, or a setting could name either.
  for (std::size_t at = 0u; at < providers_offered.size(); ++at) {
    EXPECT_TRUE(!providers_offered[at].id.empty());
    EXPECT_TRUE(providers_offered[at].id.find(' ') ==
                std::string_view::npos);
    for (std::size_t other = at + 1; other < providers_offered.size();
         ++other)
      EXPECT_TRUE(providers_offered[at].id != providers_offered[other].id);
  }

  // Every door away from this page stands whole, at a host spoken to over a
  // line this program knows: an encrypted one, or this machine's own where
  // an encrypted one would be theatre -- a model run at home has no
  // certificate to show, and a key for it may cross in the clear (provider/
  // http/wire.cpp, cleartext_is_safe, says the same).
  // And whose words are spoken there: one of the three families, never
  // nothing -- a door that said nothing would leave a model nobody here has
  // heard of to be guessed at from how its base ends.
  for (const provider_choice &one : providers_offered) {
    if (!one.base.empty())
      EXPECT_TRUE(one.base.starts_with("https://") ||
                  one.base.starts_with("http://localhost:"));
    if (!one.alias.empty())
      EXPECT_TRUE(one.alias == "openai" || one.alias == "anthropic" ||
                  one.alias == "gemini");
  }

  // The door named in the settings is the door that stands, and this page's
  // own server where the word names none of them -- which is also where a
  // word from a build that offered no such door lands.
  EXPECT_EQ(std::string(provider_by("anthropic").id),
            std::string("anthropic"));
  EXPECT_EQ(std::string(provider_by("").id), std::string("page"));
  EXPECT_EQ(std::string(provider_by("gone-in-a-newer-build").id),
            std::string("page"));

  // The automatic compact, as every reader of a settings file meets it:
  // on, as Agora has it on -- and off again the moment somebody says off,
  // and kept that way.
  EXPECT_TRUE(offered_of(default_asking, switch_choice::auto_compact));
  const asking off =
      set_of(default_asking, switch_choice::auto_compact, false);
  EXPECT_FALSE(offered_of(off, switch_choice::auto_compact));
  EXPECT_TRUE(offered_of(set_of(off, switch_choice::auto_compact, true),
                         switch_choice::auto_compact));

  // A door as it is kept until one is chosen: this page's server, and
  // nothing that opens it.
  const door held;
  EXPECT_EQ(held.provider, std::string("page"));
  EXPECT_TRUE(held.holds.empty());

  // What opens a door: typed for one, kept for that one alone, and gone
  // when it is typed away -- a door with nothing typed for it is asked of
  // with nothing, which is all a door that needs no key ever has.
  door typing;
  EXPECT_EQ(key_of(typing, "ollama"), std::string{});
  key_into(typing, "ollama", "not-needed-here");
  EXPECT_EQ(key_of(typing, "ollama"), std::string("not-needed-here"));
  EXPECT_EQ(key_of(typing, "openai"), std::string{});
  key_into(typing, "openai", "sk-second-door");
  EXPECT_EQ(key_of(typing, "ollama"), std::string("not-needed-here"));
  key_into(typing, "ollama", "");
  EXPECT_EQ(key_of(typing, "ollama"), std::string{});
  EXPECT_EQ(key_of(typing, "openai"), std::string("sk-second-door"));
}

SAID(config, what_a_door_may_answer_as_is_written_apart_and_read_back) {
  // As the field writes them: ids apart, nothing between them kept, and the
  // same id asked for once however often it is written.
  const auto listed = text_to_models(
      " deepseek-chat , deepseek-reasoner ,, qwen-max ,deepseek-chat");
  EXPECT_EQ(listed.size(), 3u);
  EXPECT_EQ(listed[0], std::string("deepseek-chat"));
  EXPECT_EQ(listed[1], std::string("deepseek-reasoner"));
  EXPECT_EQ(listed[2], std::string("qwen-max"));
  EXPECT_EQ(models_to_text(listed),
            std::string("deepseek-chat, deepseek-reasoner, qwen-max"));
  // Nothing written: nothing kept, and nothing standing where it would be.
  EXPECT_TRUE(text_to_models("").empty());
  EXPECT_TRUE(text_to_models("  , , ").empty());
  EXPECT_EQ(models_to_text({}), std::string{});

  // Through the door itself: kept for the door it was written for and for
  // that one alone, and put away with its entry only once neither half of it
  // -- the key nor the models -- stands.
  door typing;
  models_into(typing, "deepseek", text_to_models("deepseek-chat"));
  EXPECT_EQ(models_of(typing, "deepseek").size(), 1u);
  EXPECT_EQ(models_of(typing, "openai").size(), 0u);
  key_into(typing, "deepseek", "sk-here");
  EXPECT_EQ(key_of(typing, "deepseek"), std::string("sk-here"));
  // The key taken away leaves the models standing: only both gone together
  // put the entry away.
  key_into(typing, "deepseek", "");
  EXPECT_EQ(key_of(typing, "deepseek"), std::string{});
  EXPECT_EQ(models_of(typing, "deepseek").size(), 1u);
  models_into(typing, "deepseek", {});
  EXPECT_TRUE(models_of(typing, "deepseek").empty());
  EXPECT_TRUE(typing.holds.empty());
}

int main() { return pchat::tests::run(); }
