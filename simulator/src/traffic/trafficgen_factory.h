#ifndef SIMULATOR_TRAFFIC_TRAFFICGEN_FACTORY_H
#define SIMULATOR_TRAFFIC_TRAFFICGEN_FACTORY_H

// Library headers
#include "trafficgen.h"

// Libconfig
#include <libconfig.h++>

/**
 * Factory class for instantiating traffic generators.
 */
class TrafficGeneratorFactory final {
public:
    /**
     * Returns a traffic-gen corresponding
     * to the parameterized configuration.
     */
    static TrafficGenerator*
    generate(const bool is_dry_run,
             const TrafficClass tg_type,
             const libconfig::Setting& tg_config,
             const uint32_t fid_start_offset = 0);
};

#endif // SIMULATOR_TRAFFIC_TRAFFICGEN_FACTORY_H
