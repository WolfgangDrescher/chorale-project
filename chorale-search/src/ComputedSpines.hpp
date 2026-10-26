#pragma once

#include <string_view>

#include "humlib.h"

namespace choralesearch {

// The exclusive interpretations of a chorale of four voices with the computed spines, as in the corpus.
constexpr std::string_view kExclusiveInterpretationsWithComputedSpines =
    "**kern\t**metweight\t**mint\t**deg\t**fb\t**hint-12\t**hint-13\t**hint-14\t"
    "**kern\t**metweight\t**mint\t**deg\t**hint-23\t**hint-24\t"
    "**kern\t**metweight\t**mint\t**deg\t**hint-34\t"
    "**kern\t**metweight\t**mint\t**deg";

// Adds **deg, **mint, **metweight, **fb and **hint-xy to `infile`. Throws if a tool changes the lines.
void addComputedSpines(hum::HumdrumFile& infile);

// Adds the computed spines where `infile` has only **kern, keeps a file that has them all, and
// throws for any other spines.
void addComputedSpinesWhereMissing(hum::HumdrumFile& infile);

} // namespace choralesearch
