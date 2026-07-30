#pragma once

#include "humlib.h"

namespace choralesearch {

// Adds the derived analysis spines (**deg, **mint, **metweight, **fb and the per-voice-pair
// **hint-xy) to `infile`, in place. Throws std::runtime_error if one of the tools changes the
// number of lines in the file.
void applySpineAnalysisTools(hum::HumdrumFile& infile);

} // namespace choralesearch
