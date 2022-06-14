#include "utils.h"

// Library headers
#include "macros.h"

// STD headers
#include <math.h>

/**
 * Helper function. Given a packet rate and expected packet size,
 * returns the inter-arrival time for the corresponding traffic.
 */
double getTrafficInterArrivalTimeInNs(
    const double traffic_rate_in_bits_per_sec,
    const uint32_t expected_packet_size_in_bits) {
    // Disable traffic generation
    if (traffic_rate_in_bits_per_sec == 0) { return kDblPosInfty; }

    return ((kNanosecsPerSec * expected_packet_size_in_bits) /
            traffic_rate_in_bits_per_sec);
}

/**
 * Helper string parsing functions. Emulates Python's str.split().
 */
bool endsWith(const std::string& s, const std::string& suffix) {
    return (s.size() >= suffix.size() &&
            s.substr(s.size() - suffix.size()) == suffix);
}

std::vector<std::string> split(const std::string& s,
                               const std::string& delimiter) {
    std::vector<std::string> tokens;
    for (size_t start = 0, end; start < s.length();
         start = end + delimiter.length()) {
        size_t position = s.find(delimiter, start);
        end = position != std::string::npos ? position : s.length();

        std::string token = s.substr(start, end - start);
        tokens.push_back(token);
    }
    if (s.empty() || endsWith(s, delimiter)) {
        tokens.push_back("");
    }
    return tokens;
}

/**
 * Helper function. Returns whether a and b are equal
 * (within floating-point error margin).
 */
bool DoubleApproxEqual(const double a, const double b,
                       const double epsilon) {
    return fabs(a - b) < epsilon;
}
