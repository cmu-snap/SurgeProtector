#include "normal_distribution.h"

// STD headers
#include <iomanip>
#include <iostream>

/**
 * NormalDistribution implementation.
 */
NormalDistribution::NormalDistribution(
    const double mu, const double sigma,
    const double min, const double max) :
    Distribution(name(), min, max), dist_(mu, sigma) {
    updateSampleParameters();
}

void NormalDistribution::printConfiguration() const {
    std::cout << "{ type: " << name() << ", "
              << std::fixed << std::setprecision(2)
              << "min: " << min() << ", "
              << "max: " << max() << ", "
              << "mu: " << kSampleStats.getMean() << ", "
              << "sigma: " << kSampleStats.getStd() << " }";
}

// Internal helper method. Computes sample statistics
// for the (possibly truncated) Gaussian distribution.
void NormalDistribution::updateSampleParameters() {
    Statistics sample_stats{dist_.mean(), dist_.stddev()};

    // Truncated distribution
    if (isTruncated()) {
        constexpr size_t kMaxNumSamples = 1000000;
        std::vector<double> v(kMaxNumSamples, 0); // Samples
        for (size_t idx = 0; idx < v.size(); idx++) { v[idx] = sample(); }
        sample_stats = analyzeSamples(v); // Compute the sample statistics
    }
    // Update the sample stats
    kSampleStats = sample_stats;
}

/**
 * Returns whether this a truncated Gaussian.
 */
bool NormalDistribution::isTruncated() const {
    return (kMin != kDblNegInfty || kMax != kDblPosInfty);
}

/**
 * Sample from the distribution.
 */
double NormalDistribution::sample() {
    while (true) {
        double sample = dist_(generator_);
        if (sample >= kMin && sample <= kMax) { return sample; }
    }
}
