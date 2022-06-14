#include "distribution_factory.h"

// Library headers
#include "constant_distribution.h"
#include "exponential_distribution.h"
#include "normal_distribution.h"
#include "uniform_distribution.h"

// STD headers
#include <string>
#include <stdexcept>

/**
 * DistributionFactory implementation.
 */
Distribution* DistributionFactory::
generate(const libconfig::Setting& dist_config) {
    Distribution* distribution = nullptr;

    std::string type; // Distribution type
    if (!dist_config.lookupValue("type", type)) {
        throw std::runtime_error("No distribution type specified.");
    }
    // Constant distribution
    else if (type == ConstantDistribution::name()) {
        double value; // Constant value
        if (!dist_config.lookupValue("value", value)) {
            throw std::runtime_error(
                "Must specify 'value' for a constant distribution.");
        }
        else { distribution = new ConstantDistribution(value); }
    }
    // Exponential distribution
    else if (type == ExponentialDistribution::name()) {
        double rate; // Rate of exponential
        if (!dist_config.lookupValue("rate", rate)) {
            throw std::runtime_error(
                "Must specify 'rate' for an exponential distribution.");
        }
        else { distribution = new ExponentialDistribution(rate); }
    }
    // Normal distribution
    else if (type == NormalDistribution::name()) {
        double mu, sigma; // Normal parameters
        double min = kDblNegInfty; // Min value
        double max = kDblPosInfty; // Max value

        if (!dist_config.lookupValue("mu", mu) ||
            !dist_config.lookupValue("sigma", sigma)) {
            throw std::runtime_error(
                "Must specify 'mu' and 'sigma' for a normal distribution.");
        }
        else {
            dist_config.lookupValue("min", min);
            dist_config.lookupValue("max", max);
            distribution = new NormalDistribution(mu, sigma, min, max);
        }
    }
    // Uniform distribution
    else if (type == UniformDistribution::name()) {
        double a, b; // Distribution parameters
        if (dist_config.lookupValue("lower", a) &&
            dist_config.lookupValue("upper", b)) {
            distribution = new UniformDistribution(a, b);
        }
        else if (dist_config.lookupValue("mean", a) &&
                 dist_config.lookupValue("std", b)) {
            Distribution::Statistics stats(a, b);
            distribution = UniformDistribution::from(stats);
        }
        else {
            throw std::runtime_error(
                "Must specify either ('lower', 'upper') "
                "OR ('mean', 'std') for a uniform distribution.");
        }
    }
    // Unknown distribution
    else { throw std::runtime_error(
        "Unknown distribution type: " + type + "."); }

    return distribution;
}
