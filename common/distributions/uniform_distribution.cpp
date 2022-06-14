#include "uniform_distribution.h"

// STD headers
#include <iomanip>
#include <iostream>

UniformDistribution::
UniformDistribution(const double a, const double b) :
                    Distribution(name(), a, b), dist_(a, b) {
    double mean = (a + b) / 2;
    double std = (b - a) / sqrt(12);

    // Update sample stats
    kSampleStats.set(mean, std);
}

UniformDistribution*
UniformDistribution::from(const Distribution::Statistics stats) {
    double b = stats.getMean() + (sqrt(3) * stats.getStd());
    double a = (2 * stats.getMean()) - b;

    // Return a distribution with the given parameters
    return new UniformDistribution(a, b);
}

void UniformDistribution::printConfiguration() const {
    std::cout << "{ type: " << name() << ", "
              << "lower: " << min() << ", "
              << "upper: " << max() << " }";
}
