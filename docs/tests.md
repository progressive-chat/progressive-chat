# Every test in the tree, and what each one asks

Nothing here is asked of the glass: what the page drew is what `tools/` asks
([probes.md](probes.md)). What is asked here is everything that can be asked
without a window -- a function handed a thing and what it says back -- and
everything that can be asked without a wire, which is most of the protocol.

Every file is one binary of its own (`pchat-<file>-test`, made in
`test/CMakeLists.txt` from the file's name, so that a file added there is run
without saying it twice), every binary is run by the one target `pchat-tests`,
and every build runs that: `check` in CI, and `ctest` on the machine. No test
keeps anything from one that was run before it.

## What no test says

- **No window.** Nothing in `test/` needs one drawn, and nothing in it is
  drawn: this is why everything a screen does is either in `src/logic/` --
  asked without one -- or read off the built page instead.
- **No wire.** Nothing opens anything. What would be carried is handed to a
  `pchat.platform.carry` that answers out of `test/`'s own strings, which is
  how a stream cut anywhere, a failure inside one, and a body written for one
  of three wires are all asked here.
- **No machine.** No test asks the machine it runs on, and no test is written
  twice for two machines.

## What every file asks

The name of a test says what it asks, the way the rest of this tree is
written: `SAID(group, what_is_asked)`. The group and the file are one thing.

### `test/core_test.cc` -- what a conversation is

| test | what it asks |
| --- | --- |
| `ids_compare_by_value` | two ids said the same are one id, and not two that stand beside each other |
| `root_keeps_what_it_was_given` | what was put into `chats_root` is what is read out of it, whole |
| `delivery_is_ordered_as_it_is_lived` | the five deliveries are ordered as they are lived, so that one can be said to be behind another |

### `test/json_test.cc` -- what is read and written of a wire

| test | what it asks |
| --- | --- |
| `a_thing_is_read_and_its_members_are_found` | a body read, and a member found in it by its name |
| `what_the_writer_escapes_comes_back_whole` | a quote, a backslash and a line broken inside a string come back as they went in |
| `a_number_a_truth_and_nothing_are_read_as_each` | a number is a number, `true` a truth, and `null` nothing at all |
| `where_reading_gave_up_is_said` | a body that is not whole says where reading gave up, rather than reading on |
| `what_is_written_is_read_back_the_same` | what the writer wrote, read again, is what it wrote |
| `a_list_of_numbers_and_a_member_nothing` | a list is read as a list, and a member not there is asked for without failing |

### `test/logic_test.cc` -- what the drawing asks of the model

| test | what it asks |
| --- | --- |
| `pinned_first_then_the_last_spoken_in` | a pinned conversation is said first, and after them the one spoken in last first of the rest |
| `a_search_matches_a_title_in_any_case` | what the field in the drawer names is found whatever case it is written in |
| `the_badge_counts_conversations_and_messages` | how many conversations there are, and how many messages in each, said as one badge count |
| `sending_makes_one_message_and_an_empty_field` | what sending the field says: one message said, and the field empty again |
| `marks_make_runs_of_style` | a line with marks in it read as the runs each mark wraps |
| `the_links_in_a_line_and_where_each_goes` | every place named in a line, and where each of them begins and ends in what is drawn |
| `what_the_marks_said_and_where_each_run_stands` | the line without its marks, and where each run stands in what is left |
| `a_link_written_the_way_writing_writes_one` | what is called in square stops and where it goes in round ones read as one place and its target |
| `a_line_said_in_quotes_and_how_deep_it_stands` | stops of quote at the head of a line taken out, and how deep each line stands said of them |
| `a_stretch_struck_out_says_what_it_said` | two stops of tilde read as one run struck out, and one tilde alone left as words |
| `a_stretch_said_underlined_and_one_said_slanted` | two stops of underscore read as a line under it, and one as slanted, whichever of the two is read first |
| `three_backticks_say_a_block_and_nothing_of_themselves` | three backticks at the head of a line read as a block of code: neither fence said, nor the word on the first, and no mark inside it read |
| `doc.hashes_counted_and_a_list_told_from_words` | the design's own words read: how many hashes a heading is, and a list read out of what its lines say |
| `doc.a_last_line_with_no_break_after_it` | the last line of a stretch read even where nothing breaks after it |
| `doc.a_mark_alone_in_front_is_words` | a mark alone in front of a line is words, and not the head of a run |
| `doc.the_design_this_build_was_made_with_is_read_whole` | the design this was written from is read whole and none of it is lost in the reading |

### `test/theme_test.cc` -- the colours, and what an answer is asked with

| test | what it asks |
| --- | --- |
| `theme.every_preset_gives_a_scheme` | every preset gives a scheme with every colour it is asked for |
| `theme.amoled_blacks_out_the_background` | the amoled preset blacks out what is behind everything else |
| `theme.looks_give_colours_for_the_mode` | the same looks give light or dark as the mode asks, and a scheme for either |
| `theme.what_an_answer_is_asked_with_keeps_inside_its_lines` | what an answer is asked with is held inside the lines it may not go past |
| `config.an_answerer_is_put_in_and_taken_out_of_a_chat` | an answerer put into a conversation, and taken out of one again |

### `test/tokens_test.cc` -- what what was said is worth

| test | what it asks |
| --- | --- |
| `silence_is_worth_nothing` | nothing said is worth nothing at all |
| `something_said_is_worth_something` | something said is worth more than nothing |
| `the_same_text_costs_the_same_twice` | the same text costs the same however many times it is counted |
| `more_text_costs_more` | more of it costs more than less of it |
| `another_writing_is_priced_by_the_stroke` | a text written in another writing is priced by its own strokes |
| `a_handed_in_vocabulary_answers_from_here` | a vocabulary handed in answers from itself and nowhere else |
| `the_carried_vocabulary_prices_what_the_reference_prices` | the vocabulary carried in prices what the reference prices, or as near as makes no difference |

### `test/tools_test.cc` -- the three tools of this program's own

| test | what it asks |
| --- | --- |
| `what_is_owned_is_said_in_the_shape_a_wire_reads` | each tool owned said in the shape a wire reads a tool in |
| `what_was_said_is_read_back` | what a tool was asked answered back whole |
| `nothing_said_about_nothing_is_not_a_failure` | a tool asked about nothing is answered, and not failed |
| `a_call_that_named_nothing_badly_answers_plainly` | a call that named what it wanted badly is answered plainly, rather than failed for it |
| `how_much_was_said_is_counted` | what a tool answered counted, as everything said is |
| `the_time_it_was_when_it_was_asked` | what it was when a tool was asked |
| `nothing_is_answered_for_a_tool_that_is_not_ours` | a tool that is not this program's own is answered nothing at all |
| `what_the_reader_switched_off_is_not_offered` | a tool the reader switched off is not offered to any model |

### `test/provider_test.cc` -- what goes out, and what comes back

| test | what it asks |
| --- | --- |
| `a_body_is_written_for_a_wire_and_read_back_whole` | a body written for a wire read back as what went in it |
| `how_the_program_asks_is_said_in_the_body` | how hard and how far the answer may wander said in the body itself |
| `a_body_asked_for_nothing_about_choosing_says_nothing_of_it` | a body that asked for nothing about choosing says nothing of it |
| `a_second_wire_is_known_by_how_its_base_ends` | which of the three shapes a base asks for, read off how its base ends |
| `what_the_model_keeps_is_turns_on_a_wire` | what the program keeps said as the turns a wire reads |
| `a_stream_is_read_whole` | an answer arriving a frame at a time read as one answer |
| `a_stream_cut_anywhere_says_the_same` | the same stream cut anywhere in the middle says the same thing |
| `what_the_stream_said_frame_by_frame` | each frame read as it arrived and nothing held back until the end |
| `a_frame_that_is_not_written_in_json_fails_the_stream` | a frame that is not what it should be fails the stream and says why |
| `an_error_the_wire_itself_says_fails_the_stream` | an error a wire says inside a stream fails it and is said |
| `done_without_a_reason_stops_because_it_said_stop` | an answer that stopped with no reason given stopped because it was asked to |
| `thinking_written_inside_the_answer_is_not_the_answer` | what a model thought said beside its answer and never in it |
| `a_tag_cut_in_two_by_a_frame_is_whole_when_it_is_read` | a tag broken over two frames read as one tag |
| `thinking_that_never_opened_a_tag_is_thinking_until_it_closes_one` | a thought read that began before anything was asked of it |
| `thought_left_open_when_the_stream_ends_is_thought` | a thought still open when the stream ended is thought and not text |
| `a_tool_written_in_the_answer_is_a_call_and_not_text` | what a model wrote to call a tool read as a call, and not as what it said |
| `thinking_is_asked_for_as_each_family_names_it` | thinking asked for in the words each family of models names it in |
| `thinking_turned_off_is_asked_for_nowhere` | thinking turned off asked for nowhere in any of the three shapes |

### `test/provider_anthropic_test.cc` -- the third shape

| test | what it asks |
| --- | --- |
| `one_answer_is_read_in_the_words_every_wire_uses` | an answer read out of the shape Anthropic's own names |
| `a_failure_the_wire_reports_inside_the_stream_says_why` | a failure reported inside that stream is failed, and says why |

### `test/provider_run_test.cc` -- one answer, from asked to over

| test | what it asks |
| --- | --- |
| `what_is_asked_is_carried_whole_and_read_back` | what was asked carried whole, and read back as it went |
| `one_answer_handed_over_whole_is_read_whole` | an answer handed over whole read whole |
| `nothing_is_taken_before_it_is_carried` | nothing taken before it was carried anywhere |
| `an_answer_the_reader_stopped_takes_nothing_more` | an answer the reader stopped takes nothing more off the wire |
| `what_could_not_be_carried_says_why` | an answer that could not be carried says why, and is failed |
| `a_frame_that_is_not_written_in_json_fails_it_as_it_arrives` | a frame that is not whole failed as it arrives and not later |
| `the_answer_as_the_model_keeps_it` | the answer kept as the model keeps it, thought and calls and all |
| `an_answer_that_never_came_is_a_failed_message` | an answer that never came said as a message that failed |

### `test/provider_loop_test.cc` -- asking again until nothing is asked

| test | what it asks |
| --- | --- |
| `a_question_that_asked_for_nothing_is_answered_once` | a model that stopped to ask for nothing asked once and not twice |
| `what_a_tool_answered_goes_back_into_the_asking` | what a tool answered put back into what is asked next |
| `a_call_the_wire_gave_no_name_to_is_named_here` | a call a wire gave no name to named here, so that it can be answered |
| `it_stops_asking_after_as_many_rounds_as_it_may` | asking stops after as many rounds as it may ask, and not after one more |
| `an_answer_that_could_not_be_had_is_not_asked_again` | an answer that failed is not asked again |
| `an_answer_the_reader_stopped_is_not_asked_again` | an answer the reader stopped is not asked again either |

## How one is written

```cpp
SAID(logic, what_one_more_thing_comes_to) {
  const logic::drawn said = logic::drawn_of("said this way");
  EXPECT_EQ(said.said, std::string("said this way"));
}
```

`SAID(group, name)` puts one function in the runner under that name
(`test/tests.h`); `EXPECT_EQ`, `EXPECT_TRUE` and `EXPECT_NEAR` are what it is
asked with -- `EXPECT_EQ`, `EXPECT_NE`, `EXPECT_LT`, `EXPECT_GE`,
`EXPECT_TRUE`, `EXPECT_FALSE` -- and a failure says the file, the line and
both sides. The group
is the module asked, or the part of one. The name is what is asked of it --
said as a sentence, the way every commit here is written -- and not the name
of the function: a test named `test_links` says nothing to whoever reads the
line it failed on, and one named `a_place_said_in_a_line_goes_where_it_says`
does.

What is written beside a test is written the same way everything else here
is: what is true of the thing, and not what the test does. New tests go in
the file that asks that side of the tree, which is the file whose group they
carry.
