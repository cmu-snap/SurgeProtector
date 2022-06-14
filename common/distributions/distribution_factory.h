#ifndef COMMON_DISTRIBUTIONS_DISTRIBUTION_FACTORY_H
#define COMMON_DISTRIBUTIONS_DISTRIBUTION_FACTORY_H

// Library headers
#include "distribution.h"

// Libconfig
#include <libconfig.h++>

/**
 * Factory class for generating distributions.
 */
class DistributionFactory final {
public:
    /**
     * Returns a distribution corresponding
     * to the parameterized configuration.
     */
    static Distribution*
    generate(const libconfig::Setting& dist_config);
};

#endif // COMMON_DISTRIBUTIONS_DISTRIBUTION_FACTORY_H
