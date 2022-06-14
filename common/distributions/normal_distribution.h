#ifndef COMMON_DISTRIBUTIONS_NORMAL_DISTRIBUTION_H
#define COMMON_DISTRIBUTIONS_NORMAL_DISTRIBUTION_H

// Library headers
#include "distribution.h"

/**
 * Represents a normal distribution.
 */
class NormalDistribution : public Distribution {
private:
    // Implementation
    std::normal_distribution<double> dist_;

    // Internal helper methods
    void updateSampleParameters();

public:
    virtual ~NormalDistribution() {}
    explicit NormalDistribution(const double mu, const double sigma,
                                const double min=kDblNegInfty,
                                const double max=kDblPosInfty);
    /**
     * Print the distribution configuration.
     */
    virtual void printConfiguration() const override;

    // Accessors
    bool isTruncated() const;
    double sample() override;

    /**
     * Distribution name.
     */
    static std::string name() { return "normal"; }
};

#endif // COMMON_DISTRIBUTIONS_NORMAL_DISTRIBUTION_H
