#ifndef COMMON_DISTRIBUTIONS_DISTRIBUTION_H
#define COMMON_DISTRIBUTIONS_DISTRIBUTION_H

// Library headers
#include "common/macros.h"
#include "common/utils.h"

// STD headers
#include <assert.h>
#include <string>
#include <random>
#include <vector>

/**
 * Base class representing a statistical distribution.
 */
class Distribution {
public:
    // Distribution statistics
    class Statistics final {
    private:
        double mean_ = NAN; // Expectation
        double std_ = NAN; // Standard deviation

    public:
        explicit Statistics() {}
        explicit Statistics(const double mean,
                            const double std) :
                            mean_(mean), std_(std) {}
        // Mutators
        void set(const double mean,
                 const double std) { mean_ = mean; std_ = std; }

        // Accessors
        double getStd() const { return std_; }
        double getMean() const { return mean_; }
        bool isInitialized() const { return (!std::isnan(mean_) &&
                                             !std::isnan(std_)); }
    };

protected:
    // Distribution parameters
    const std::string kType;
    Statistics kSampleStats;
    const double kMin = kDblNegInfty; // Minimum value (default: -inf)
    const double kMax = kDblPosInfty; // Maximum value (default: +inf)

    // Randomness
    std::random_device rd_;
    std::mt19937 generator_;

    /**
     * Helper method. Given an array of samples (from some
     * distribution), returns their sample mean and STD.
     */
    static Statistics analyzeSamples(const std::vector<double>& v);

    // Constructors
    explicit Distribution(const std::string type) :
                          kType(type), generator_(rd_()) {}

    explicit Distribution(const std::string type, const double
                          min, const double max) : kType(type),
                          kMin(min), kMax(max), generator_(rd_()) {}
public:
    virtual ~Distribution() { assert(kSampleStats.isInitialized()); }
    DISALLOW_COPY_AND_ASSIGN(Distribution);

    // Accessors
    double min() const { return kMin; }
    double max() const { return kMax; }
    const std::string& type() const { return kType; }
    Statistics getSampleStats() const { return kSampleStats; }

    /**
     * Print the distribution configuration.
     */
    virtual void printConfiguration() const = 0;

    /**
     * Sample from the distribution.
     */
    virtual double sample() = 0;
};

#endif // COMMON_DISTRIBUTIONS_DISTRIBUTION_H
