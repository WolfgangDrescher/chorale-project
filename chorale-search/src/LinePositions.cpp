#include "LinePositions.hpp"

#include <sstream>

namespace choralesearch {

namespace {

// The beat as the annotations write it: "3", or "3+1/2" with the part of a beat as a fraction.
std::string formatBeat(hum::HumNum beat) {
    std::ostringstream text;
    beat.printMixedFraction(text, "+");
    return text.str();
}

// How much the pickup, as humlib finds it, is short of a measure of the meter, in quarter notes.
// Zero without a pickup.
hum::HumNum pickupQuarters(hum::HumdrumFile& infile) {
    const int barline = infile.hasPickup();
    for (int i = 0; i < barline; ++i) {
        if (infile[i].isInterpretation() && infile.token(i, 0)->isTimeSignature()) {
            return hum::Convert::timeSigToDurationInQuarters(infile.token(i, 0)) - infile[barline].getDurationFromStart();
        }
    }
    return 0;
}

} // namespace

std::vector<LinePosition> linePositions(hum::HumdrumFile& infile) {
    std::vector<LinePosition> positions;
    int measure = 0;
    hum::HumNum quarters = pickupQuarters(infile);
    for (int i = 0; i < infile.getLineCount(); ++i) {
        if (infile[i].isBarline()) {
            int number = infile.getMeasureNumber(i);
            if (number >= 0) {
                measure = number;
                quarters = 0;
            }
            continue;
        }
        if (!infile[i].isData()) continue;
        LinePosition position;
        position.line = infile[i].getLineNumber();
        position.measure = measure;
        position.beat = quarters + 1;
        position.position = std::to_string(measure) + "/" + formatBeat(position.beat);
        positions.push_back(position);
        quarters += infile[i].getDuration();
    }
    return positions;
}

} // namespace choralesearch
