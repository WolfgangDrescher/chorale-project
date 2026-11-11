#include "Intervals.hpp"

#include "ValueScanner.hpp"

#include <utility>

namespace choralesearch {

std::optional<IntervalParts> parseMintValue(const std::string& s) {
    ValueScanner scan(s);
    std::string direction;
    if (scan.accept('+')) direction = "+";
    else if (scan.accept('-')) direction = "-";
    std::string quality = scan.letters();
    std::string number = scan.digits();
    if (!scan.atEnd()) return std::nullopt;
    return IntervalParts{std::move(direction), std::move(quality), std::move(number)};
}

std::optional<FbValue> parseFbValue(const std::string& s) {
    ValueScanner scan(s);
    std::string quality = scan.letters();
    std::string figure = scan.digits();
    if (figure.empty() || !scan.atEnd()) return std::nullopt;
    return FbValue{std::move(quality), std::move(figure)};
}

} // namespace choralesearch
