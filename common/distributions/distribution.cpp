#include "distribution.h"

// STD headers
#include <algorithm>

Distribution::Statistics
Distribution::analyzeSamples(const std::vector<double>& v) {
    double sum = std::accumulate(std::begin(v), std::end(v), 0.0);
    double mean =  sum / v.size(); // Sample mean

    double accum = 0.0;
    std::for_each (std::begin(v), std::end(v), [&](const
        double d) { accum += (d - mean) * (d - mean); });
    double std = sqrt(accum / (v.size() - 1)); // Sample STD

    return Statistics(mean, std);
}
