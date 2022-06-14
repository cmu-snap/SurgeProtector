#ifndef COMMON_DISTRIBUTIONS_CONSTANT_DISTRIBUTION_H
#define COMMON_DISTRIBUTIONS_CONSTANT_DISTRIBUTION_H

// Library headers
#include "distribution.h"

/**
 * Represents a constant distribution.
 */
class ConstantDistribution : public Distribution {
public:
    explicit ConstantDistribution(const double mean) :
        Distribution(name(), mean, mean) {
        kSampleStats.set(mean, 0);
    }
    virtual ~ConstantDistribution() {}

    /**
     * Print the distribution configuration.
     */
    virtual void printConfiguration() const override;

    /**
     * Sample from the distribution.
     */
    double sample() override { return kSampleStats.getMean(); }

    /**
     * Distribution name.
     */
    static std::string name() { return "constant"; }
};

#endif // DISTRIBUTIONS_CONSTANT_DISTRIBUTION_H
