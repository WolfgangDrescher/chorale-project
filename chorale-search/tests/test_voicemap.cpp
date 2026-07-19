#include "test_framework.hpp"

#include "VoiceMap.hpp"

using choralesearch::resolveVoices;
using choralesearch::voiceLabel;

TEST_CASE(voice_label_maps_1_to_4) {
    CHECK_EQ(voiceLabel(1), std::string("Bass"));
    CHECK_EQ(voiceLabel(2), std::string("Tenor"));
    CHECK_EQ(voiceLabel(3), std::string("Alto"));
    CHECK_EQ(voiceLabel(4), std::string("Soprano"));
}

TEST_CASE(voice_label_out_of_range_is_question_mark) {
    CHECK_EQ(voiceLabel(0), std::string("?"));
    CHECK_EQ(voiceLabel(5), std::string("?"));
}

TEST_CASE(resolve_voices_empty_or_all_returns_every_voice) {
    std::vector<std::size_t> expected = {1, 2, 3, 4};
    CHECK_EQ(resolveVoices(""), expected);
    CHECK_EQ(resolveVoices("all"), expected);
    CHECK_EQ(resolveVoices("ALL"), expected);
}

TEST_CASE(resolve_voices_single_digit) {
    std::vector<std::size_t> expected = {4};
    CHECK_EQ(resolveVoices("4"), expected);
}

TEST_CASE(resolve_voices_multi_digit_token_expands_and_sorts) {
    std::vector<std::size_t> expected = {1, 2, 4};
    CHECK_EQ(resolveVoices("124"), expected);

    std::vector<std::size_t> allFour = {1, 2, 3, 4};
    CHECK_EQ(resolveVoices("4321"), allFour);
}

TEST_CASE(resolve_voices_names_case_insensitive) {
    std::vector<std::size_t> expected = {4};
    CHECK_EQ(resolveVoices("soprano"), expected);
    CHECK_EQ(resolveVoices("SOPRANO"), expected);
    CHECK_EQ(resolveVoices("Soprano"), expected);
}

TEST_CASE(resolve_voices_single_letter_names) {
    CHECK_EQ(resolveVoices("b"), std::vector<std::size_t>{1});
    CHECK_EQ(resolveVoices("t"), std::vector<std::size_t>{2});
    CHECK_EQ(resolveVoices("a"), std::vector<std::size_t>{3});
    CHECK_EQ(resolveVoices("s"), std::vector<std::size_t>{4});
}

TEST_CASE(resolve_voices_comma_separated_list_dedupes_and_sorts) {
    std::vector<std::size_t> expected = {1, 3, 4};
    CHECK_EQ(resolveVoices("soprano,bass,alto"), expected);
    CHECK_EQ(resolveVoices("soprano, bass , alto"), expected);
    CHECK_EQ(resolveVoices("soprano,soprano,bass"), (std::vector<std::size_t>{1, 4}));
}

TEST_CASE(resolve_voices_mixes_digit_tokens_and_names) {
    std::vector<std::size_t> expected = {1, 2, 3, 4};
    CHECK_EQ(resolveVoices("12,alto,soprano"), expected);
}

TEST_CASE(resolve_voices_rejects_unknown_name) {
    CHECK_THROWS(resolveVoices("baritone"));
}

TEST_CASE(resolve_voices_rejects_out_of_range_digit) {
    CHECK_THROWS(resolveVoices("5"));
    CHECK_THROWS(resolveVoices("0"));
}

TEST_MAIN()
