#pragma once

#include <string>
#include <vector>

#include "humlib.h"

namespace choralesearch {

// Where a data line is, as annotations/bach-modulations.json writes it: "measure/beat" in quarters
// from the last numbered barline, a pickup counted back from the first barline ("0/4").
struct LinePosition {
    int line = 0; // counted from 1, as humlib numbers them
    int measure = 0;
    hum::HumNum beat;
    std::string position; // "measure/beat", the part of a beat as a fraction: "3", "3+1/2"
};

// The position of every data line of the score, in order.
std::vector<LinePosition> linePositions(hum::HumdrumFile& infile);

} // namespace choralesearch
