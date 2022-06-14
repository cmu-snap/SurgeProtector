#ifndef SIMULATOR_APPLICATIONS_APPLICATION_FACTORY_H
#define SIMULATOR_APPLICATIONS_APPLICATION_FACTORY_H

// Library headers
#include "application.h"

// Libconfig
#include <libconfig.h++>

/**
 * Factory class for instantiating applications.
 */
class ApplicationFactory final {
public:
    /**
     * Returns an application corresponding
     * to the parameterized configuration.
     */
    static Application*
    generate(const libconfig::Setting& app_config);
};

#endif // SIMULATOR_APPLICATIONS_APPLICATION_FACTORY_H
