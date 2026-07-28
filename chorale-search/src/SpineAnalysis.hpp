#pragma once

#include "humlib.h"

namespace choralesearch {

// Adds the derived analysis spines (**deg, **mint, **metweight, **fb and the per-voice-pair
// **hint-xy) to `infile`, in place.
void applySpineAnalysisTools(hum::HumdrumFile& infile);

} // namespace choralesearch
