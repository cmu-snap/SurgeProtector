#include "constant_distribution.h"

// STD headers
#include <iomanip>
#include <iostream>

/**
 * ConstantDistribution implementation.
 */
void ConstantDistribution::printConfiguration() const {
    std::cout << "{ type: " << name() << ", "
              << std::fixed << std::setprecision(2)
              << "value: " << kSampleStats.getMean() << " }";
}
