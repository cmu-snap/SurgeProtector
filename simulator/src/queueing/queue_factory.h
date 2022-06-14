#ifndef SIMULATOR_QUEUEING_QUEUE_FACTORY_H
#define SIMULATOR_QUEUEING_QUEUE_FACTORY_H

// Library headers
#include "base_queue.h"

// Libconfig
#include <libconfig.h++>

/**
 * Factory class for instantiating queues.
 */
class QueueFactory final {
public:
    /**
     * Returns a queue corresponding to the queueing
     * policy specified in the given configuration.
     */
    static BaseQueue*
    generate(const libconfig::Setting& queue_config);
};

#endif // SIMULATOR_QUEUEING_QUEUE_FACTORY_H
