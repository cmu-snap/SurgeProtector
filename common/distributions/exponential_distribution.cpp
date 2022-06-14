#include "exponential_distribution.h"

// STD headers
#include <iomanip>
#include <iostream>

/**
 * ExponentialDistribution implementation.
 */
void ExponentialDistribution::printConfiguration() const {
    std::cout << "{ type: " << name() << ", "
              << std::fixed << std::setprecision(2)
              << "rate: " << 1 / kSampleStats.getMean() << " }";
}
