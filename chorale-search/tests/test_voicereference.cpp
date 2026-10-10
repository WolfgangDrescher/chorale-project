#include "test_framework.hpp"

#include "VoiceReference.hpp"

using choralesearch::isValidVoiceReference;
using choralesearch::ReferenceQuantifier;
using choralesearch::resolveVoiceReference;

namespace {

std::vector<std::size_t> voicesOf(const std::string& text, std::size_t walkedVoice) {
    auto resolved = resolveVoiceReference(text, walkedVoice);
    return resolved ? resolved->voices : std::vector<std::size_t>{99};
}

} // namespace

TEST_CASE(voice_reference_names_are_the_known_spellings) {
    for (const std::string& name : {"@1", "@2", "@3", "@4", "@below", "@above", "@anyBelow", "@allBelow",
                                      "@anyAbove", "@allAbove", "@anyOther", "@allOther"}) {
        CHECK(isValidVoiceReference(name));
    }
}

TEST_CASE(voice_reference_names_reject_everything_else) {
    for (const std::string& name : {"", "@", "@0", "@5", "@12", "@Below", "@belowAll", "@sideways", "2", "below", "@ 2"}) {
        CHECK(!isValidVoiceReference(name));
        CHECK(!resolveVoiceReference(name, 2).has_value());
    }
}

TEST_CASE(voice_reference_to_a_fixed_voice_is_that_voice) {
    CHECK_EQ(voicesOf("@1", 3), (std::vector<std::size_t>{1}));
    CHECK_EQ(voicesOf("@4", 2), (std::vector<std::size_t>{4}));
}

TEST_CASE(voice_reference_to_the_walked_voice_itself_names_no_voice) {
    CHECK(voicesOf("@3", 3).empty());
}

TEST_CASE(voice_reference_to_a_neighbour_follows_the_walked_voice) {
    CHECK_EQ(voicesOf("@below", 3), (std::vector<std::size_t>{2}));
    CHECK_EQ(voicesOf("@above", 3), (std::vector<std::size_t>{4}));
    CHECK(voicesOf("@below", 1).empty()); // nothing below the bass
    CHECK(voicesOf("@above", 4).empty()); // nothing above the soprano
}

TEST_CASE(voice_reference_to_a_group_lists_its_voices) {
    CHECK_EQ(voicesOf("@anyBelow", 4), (std::vector<std::size_t>{1, 2, 3}));
    CHECK_EQ(voicesOf("@allAbove", 2), (std::vector<std::size_t>{3, 4}));
    CHECK_EQ(voicesOf("@anyOther", 3), (std::vector<std::size_t>{1, 2, 4}));
    CHECK(voicesOf("@allBelow", 1).empty());
    CHECK(voicesOf("@anyAbove", 4).empty());
}

TEST_CASE(voice_reference_combines_a_group_by_its_name) {
    // The nearest voice is one voice, for which any and all say the same.
    CHECK(resolveVoiceReference("@below", 3)->quantifier == ReferenceQuantifier::Any);
    CHECK(resolveVoiceReference("@above", 3)->quantifier == ReferenceQuantifier::Any);
    CHECK(resolveVoiceReference("@anyBelow", 3)->quantifier == ReferenceQuantifier::Any);
    CHECK(resolveVoiceReference("@anyOther", 3)->quantifier == ReferenceQuantifier::Any);
    CHECK(resolveVoiceReference("@allBelow", 3)->quantifier == ReferenceQuantifier::All);
    CHECK(resolveVoiceReference("@allAbove", 3)->quantifier == ReferenceQuantifier::All);
    CHECK(resolveVoiceReference("@allOther", 3)->quantifier == ReferenceQuantifier::All);
}

TEST_CASE(voice_reference_of_no_voice_walked_names_no_voice) {
    CHECK(voicesOf("@anyOther", 0).empty());
    CHECK(voicesOf("@anyOther", 5).empty());
}

TEST_MAIN()
