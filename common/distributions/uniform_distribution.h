#ifndef COMMON_DISTRIBUTIONS_UNIFORM_DISTRIBUTION_H
#define COMMON_DISTRIBUTIONS_UNIFORM_DISTRIBUTION_H

// Library headers
#include "distribution.h"

/**
 * Represents a uniform distribution.
 */
class UniformDistribution : public Distribution {
private:
    // Implementation
    std::uniform_real_distribution<double> dist_;

public:
    explicit UniformDistribution(const double a, const double b);
    virtual ~UniformDistribution() {}

    /**
     * Factory method. Generates a uniform distribution
     * with the given distribution stats (mean, STD).
     */
    static UniformDistribution* from(const Statistics stats);

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
    static std::string name() { return "uniform"; }
};

#endif // COMMON_DISTRIBUTIONS_UNIFORM_DISTRIBUTION_H
