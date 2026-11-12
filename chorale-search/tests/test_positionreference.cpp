#include "test_framework.hpp"

#include <optional>
#include <string>

#include "PositionReference.hpp"
#include "QueryValidation.hpp"

using choralesearch::ComparisonOperator;
using choralesearch::PatternPosition;
using choralesearch::isValidPositionName;
using choralesearch::PatternValue;
using choralesearch::PositionReference;
using choralesearch::positionNames;
using choralesearch::resolvePositionReferences;

namespace {

// Any note, then `value` under `key`.
std::vector<PatternPosition> afterAnyNote(const std::string& key, const PatternValue& value) {
    return {PatternPosition{.keys = {{"kern", {"*"}}}}, PatternPosition{.keys = {{key, {value}}}}};
}

// The reference the first value of `key` at `position` resolves to.
std::optional<PositionReference> resolvedReference(const std::vector<PatternPosition>& pattern, std::size_t position,
                                                   const std::string& key) {
    return resolvePositionReferences(pattern)[position].keys.at(key).at(0).positionReference;
}

} // namespace

TEST_CASE(position_reference_is_a_number_or_a_name_and_nothing_else) {
    for (const char* text : {"#1", "#12", "$start", "$s", "$a1_b", "#1.mint", "$s.mint.number", "#0"}) {
        CHECK(PatternValue(text).isPositionReference());
    }
    // The accidentals of kern, a voice reference, a name without its sign, and what is neither.
    for (const char* text : {"#", "##", "#-", "@2", "start", "$", "$1", "1", "", "-"}) {
        CHECK(!PatternValue(text).isPositionReference());
    }
}

TEST_CASE(position_name_is_a_dollar_sign_a_letter_and_then_letters_digits_and_underscores) {
    for (const char* name : {"$a", "$start", "$S1", "$a_b", "$firstNote2"}) CHECK(isValidPositionName(name));
    for (const char* name : {"", "$", "$1", "$_a", "a", "#1", "$a-b", "$a.b", "$a b", "@2"}) CHECK(!isValidPositionName(name));
}

TEST_CASE(position_names_are_read_off_the_as_keys) {
    std::vector<PatternPosition> pattern = {
        PatternPosition{.keys = {{"as", {"$first"}}, {"kern", {"*"}}}},
        PatternPosition{.keys = {{"kern", {"*"}}}},
        PatternPosition{.keys = {{"as", {"$last"}}, {"kern", {"*"}}}},
    };
    auto names = positionNames(pattern);
    REQUIRE(names.size() == 3u);
    CHECK_EQ(*names[0], std::string("$first"));
    CHECK(!names[1].has_value());
    CHECK_EQ(*names[2], std::string("$last"));
    CHECK_THROWS(positionNames({PatternPosition{.keys = {{"as", {"start"}}}}}));
    CHECK_THROWS(positionNames({PatternPosition{.keys = {{"as", {"$a", "$b"}}}}}));
    CHECK_THROWS(positionNames({PatternPosition{.keys = {{"as", {"$a"}}}}, PatternPosition{.keys = {{"as", {"$a"}}}}}));
}

TEST_CASE(reference_reads_the_key_it_stands_under_parts_included) {
    std::vector<PatternPosition> pattern = {
        PatternPosition{.keys = {{"as", {"$a"}}, {"mint", {"*"}}}},
        PatternPosition{.keys = {{"kern", {"#1"}}, {"!mint.direction", {"$a"}}, {"mint.number", {PatternValue(ComparisonOperator::GreaterThan, "#1")}}}},
    };
    CHECK_EQ(resolvedReference(pattern, 1, "kern"), std::optional(PositionReference{0, "kern"}));
    CHECK_EQ(resolvedReference(pattern, 1, "!mint.direction"), std::optional(PositionReference{0, "mint.direction"}));
    CHECK_EQ(resolvedReference(pattern, 1, "mint.number"), std::optional(PositionReference{0, "mint.number"}));
    // The names stay in the pattern: what reads the pattern back out needs them.
    CHECK(resolvePositionReferences(pattern)[0].keys.count("as") == 1u);
}

TEST_CASE(reference_may_read_its_own_position_and_leaves_other_values_alone) {
    CHECK_EQ(resolvedReference({PatternPosition{.keys = {{"kern", {"*"}}, {"duration", {"#1"}}}}}, 0, "duration"), std::optional(PositionReference{0, "duration"}));
    std::vector<PatternPosition> pattern = {PatternPosition{.keys = {{"kern", {"#", "@2", "g", "*", {ComparisonOperator::LessThan, "c"}}}}}};
    const std::vector<PatternPosition> resolved = resolvePositionReferences(pattern);
    for (const PatternValue& value : resolved[0].keys.at("kern")) CHECK(!value.positionReference.has_value());
}

TEST_CASE(reference_reads_another_key_where_an_interval_or_its_number_stands) {
    CHECK_EQ(resolvedReference(afterAnyNote("fb", "#1.hint-14"), 1, "fb"), std::optional(PositionReference{0, "hint-14"}));
    CHECK_EQ(resolvedReference(afterAnyNote("fb", "#1.mint.interval"), 1, "fb"), std::optional(PositionReference{0, "mint.interval"}));
    CHECK_EQ(resolvedReference(afterAnyNote("fb.numbers", "#1.hint-2.number"), 1, "fb.numbers"), std::optional(PositionReference{0, "hint-2.number"}));
    CHECK_EQ(resolvedReference(afterAnyNote("hint-14", "#1.hint-2"), 1, "hint-14"), std::optional(PositionReference{0, "hint-2"}));
    CHECK_EQ(resolvedReference(afterAnyNote("hint-14.number", "#1.mint.number"), 1, "hint-14.number"), std::optional(PositionReference{0, "mint.number"}));
    CHECK_EQ(resolvedReference(afterAnyNote("hint-@pair", "#1"), 1, "hint-@pair"), std::optional(PositionReference{0, "hint-@pair"}));
}

TEST_CASE(reference_rejects_a_value_of_another_kind_than_its_key_takes) {
    CHECK_THROWS(resolvePositionReferences(afterAnyNote("deg", "#1.mint")));
    CHECK_THROWS(resolvePositionReferences(afterAnyNote("kern", "#1.duration")));
    CHECK_THROWS(resolvePositionReferences(afterAnyNote("mint", "#1.hint-14")));
    CHECK_THROWS(resolvePositionReferences(afterAnyNote("hint-14", "#1.mint.number")));
    CHECK_THROWS(resolvePositionReferences(afterAnyNote("hint-14.number", "#1.hint-14")));
    CHECK_THROWS(resolvePositionReferences(afterAnyNote("fb", "#1.mint")));
    CHECK_THROWS(resolvePositionReferences(afterAnyNote("word", "#1.syl")));
    CHECK_THROWS(resolvePositionReferences(afterAnyNote("mode", "#1.key")));
    // A part is a key of its own, it does not stand under its whole key.
    CHECK_THROWS(resolvePositionReferences(afterAnyNote("mint", "#1.mint.direction")));
    CHECK_THROWS(resolvePositionReferences(afterAnyNote("kern", "#1.kern.pitch")));
}

TEST_CASE(reference_rejects_a_key_that_takes_or_gives_no_reference) {
    CHECK_THROWS(resolvePositionReferences(afterAnyNote("phrase", "#1")));
    CHECK_THROWS(resolvePositionReferences(afterAnyNote("hint-*4", "#1")));
    CHECK_THROWS(resolvePositionReferences(afterAnyNote("hint-*4.number", "#1")));
    CHECK_THROWS(resolvePositionReferences(afterAnyNote("kern", "#1.phrase")));
    CHECK_THROWS(resolvePositionReferences(afterAnyNote("kern", "#1.tempo")));
    CHECK_THROWS(resolvePositionReferences(afterAnyNote("kern", "#1.")));
    CHECK_THROWS(resolvePositionReferences(afterAnyNote("mint", "#1.mint.directedNumber")));
    CHECK_THROWS(resolvePositionReferences(afterAnyNote("mint", "#1.!mint")));
}

TEST_CASE(reference_rejects_a_position_the_pattern_does_not_have_or_a_later_one) {
    CHECK_THROWS(resolvePositionReferences({PatternPosition{.keys = {{"kern", {"#0"}}}}}));
    CHECK_THROWS(resolvePositionReferences({PatternPosition{.keys = {{"kern", {"#2"}}}}}));
    CHECK_THROWS(resolvePositionReferences({PatternPosition{.keys = {{"kern", {"#1x"}}}}}));
    CHECK_THROWS(resolvePositionReferences({PatternPosition{.keys = {{"kern", {"$start"}}}}}));
    std::vector<PatternPosition> pattern = {PatternPosition{.keys = {{"kern", {"#2"}}}}, PatternPosition{.keys = {{"as", {"$b"}}, {"kern", {"*"}}}}};
    CHECK_THROWS(resolvePositionReferences(pattern));
    pattern[0].keys["kern"] = {"$b"};
    CHECK_THROWS(resolvePositionReferences(pattern));
}

TEST_MAIN()

TEST_CASE(position_reference_cannot_read_a_position_that_repeats) {
    PatternPosition run{.keys = {{"as", {"$run"}}}};
    run.repeat = choralesearch::PatternRepeat{.min = 1};
    CHECK_THROWS(resolvePositionReferences({run, PatternPosition{.keys = {{"kern", {"#1"}}}}}));
    try {
        resolvePositionReferences({run, PatternPosition{.keys = {{"kern", {"$run"}}}}});
        CHECK(false);
    } catch (const std::invalid_argument& e) {
        CHECK(std::string(e.what()).find("'pattern[1][kern]'") != std::string::npos);
        CHECK(std::string(e.what()).find("names a position that repeats") != std::string::npos);
    }
    // Its own onset a repeated position reads like any other.
    PatternPosition readsItself{.keys = {{"kern", {"*"}}, {"kern.pitch", {"#1"}}}};
    readsItself.repeat = choralesearch::PatternRepeat{.min = 1};
    CHECK_NOTHROW(resolvePositionReferences({readsItself}));
}

TEST_CASE(position_reference_in_a_sub_pattern_reads_the_positions_before_it) {
    const PatternPosition any{.keys = {{"kern", {"*"}}}};
    PatternPosition repeated;
    repeated.repeat = choralesearch::PatternRepeat{.min = 1};
    repeated.subPattern = {any, PatternPosition{.keys = {{"kern", {"#1"}}}}};
    const auto resolved = resolvePositionReferences({any, repeated});
    CHECK_EQ(*resolved[1].subPattern[1].keys.at("kern").at(0).positionReference, (PositionReference{0, "kern"}));

    repeated.subPattern = {PatternPosition{.keys = {{"kern", {"#2"}}}}};
    try {
        resolvePositionReferences({any, repeated});
        CHECK(false);
    } catch (const std::invalid_argument& e) {
        CHECK(std::string(e.what()).find("'pattern[1][subPattern][0][kern]'") != std::string::npos);
        CHECK(std::string(e.what()).find("its own subPattern") != std::string::npos);
    }
    repeated.subPattern = {PatternPosition{.keys = {{"as", {"$inner"}}, {"kern", {"*"}}}}};
    CHECK_THROWS(resolvePositionReferences({repeated}));
}
