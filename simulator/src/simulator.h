#ifndef SIMULATOR_SIMULATOR_H
#define SIMULATOR_SIMULATOR_H

// Library headers
#include "queueing/base_queue.h"
#include "server/server.h"
#include "traffic/trafficgen.h"

// Libconfig
#include <libconfig.h++>

/**
 * Implements the core simulator functionality.
 */
class Simulator final {
private:
    // Simulation config
    const bool kIsDryRun; // Dry run?
    uint64_t kMaxNumArrivals; // Max arrival count
    Server* server_ = nullptr; // Server implementation
    BaseQueue* queue_ = nullptr; // Queue implementation
    TrafficGenerator* tg_innocent_ = nullptr; // Innocent traffic-gen
    TrafficGenerator* tg_attack_ = nullptr; // Adversarial traffic-gen

    // Housekeeping
    bool done = false;

    // Helper method to parse configs
    void parseSimulationConfig(const libconfig::Setting& config);

public:
    explicit Simulator(const bool is_dry_run,
                       const libconfig::Setting& config);
    ~Simulator();

    /**
     * Print the simulation configuration.
     */
    void printConfig() const;

    /**
     * Run simulation.
     */
    void run(const bool verbose,
             const std::string packets_fp="");
};

#endif // SIMULATOR_SIMULATOR_H
