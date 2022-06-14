#ifndef COMMON_DISTRIBUTIONS_EXPONENTIAL_DISTRIBUTION_H
#define COMMON_DISTRIBUTIONS_EXPONENTIAL_DISTRIBUTION_H

// Library headers
#include "distribution.h"

// STD headers
#include <stdexcept>

/**
 * Represents an exponential distribution.
 */
class ExponentialDistribution : public Distribution {
private:
    // Implementation
    std::exponential_distribution<double> dist_;

public:
    virtual ~ExponentialDistribution() {}
    explicit ExponentialDistribution(const double rate) :
        Distribution(name(), 0, kDblPosInfty), dist_(rate) {
        if (rate <= 0) {
            throw std::invalid_argument("Rate must be positive");
        }
        const double rate_inv = (1 / rate);
        kSampleStats.set(rate_inv, rate_inv);
    }

    /**
     * Print the distribution configuration.
     */
    virtual void printConfiguration() const override;

    /**
     * Sample from the distribution.
     */
    double sample() override { return dist_(generator_); }

    /**
     * Distribution name.
     */
    static std::string name() { return "exponential"; }
};

#endif // COMMON_DISTRIBUTIONS_EXPONENTIAL_DISTRIBUTION_H
