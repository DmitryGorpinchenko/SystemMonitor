#include "format.h"

namespace Format {

std::string ElapsedTime(unsigned long seconds) {
    std::string res;

    const auto hours = seconds / 3600;
    seconds -= hours * 3600;
    if (hours < 10) {
        res += '0';
    }
    res += std::to_string(hours);
    res += ':';

    const auto minutes = seconds / 60;
    seconds -= minutes * 60;
    if (minutes < 10) {
        res += '0';
    }
    res += std::to_string(minutes);
    res += ':';

    if (seconds < 10) {
        res += '0';
    }
    res += std::to_string(seconds);

    return res;
}

} // end namespace Format
