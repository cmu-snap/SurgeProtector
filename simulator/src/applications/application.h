#ifndef SIMULATOR_APPLICATIONS_APPLICATION_H
#define SIMULATOR_APPLICATIONS_APPLICATION_H

// Library headers
#include "common/macros.h"
#include "common/utils.h"
#include "packet/packet.h"

// STD headers
#include "math.h"

/**
 * Base class representing a generic network application.
 */
class Application {
public:
    // Application parameters
    class Parameters final {
    private:
        bool use_heuristic_ = false; // Estimate job sizes?
        double service_time_scaling_ = 1; // Scaling factor
        double max_attack_job_size_ns_ = NAN; // Attacker's maximum job size

    public:
        DEFAULT_CTOR_AND_DTOR(Parameters);
        explicit Parameters(const bool uses_heuristic,
                            const double scale_factor,
                            const double max_attack_jsize) :
                            use_heuristic_(uses_heuristic),
                            service_time_scaling_(scale_factor),
                            max_attack_job_size_ns_(max_attack_jsize) {}
        // Mutators
        void setUseHeuristic(const bool v) { use_heuristic_ = v; }
        void setServiceTimeScaling(const double v) { service_time_scaling_ = v; }
        void setMaxAttackJobSizeNs(const double v) { max_attack_job_size_ns_ = v; }

        // Accessors
        bool getUseHeuristic() const { return use_heuristic_; }
        double getMaxAttackJobSizeNs() const { return max_attack_job_size_ns_; }
        double getServiceTimeScaleFactor() const { return service_time_scaling_; }
    };

protected:
    const std::string kType; // App type
    const Parameters kAppParams; // App params
    explicit Application(const std::string type,
                         const Parameters& params) :
                         kType(type), kAppParams(params) {}
    /**
     * Internal helper function. Converts from context-dependent
     * application service time to context-agnostic job size (ns).
     */
    double toJobSizeInNs(double service_time) const {
        if (service_time == kInvalidJobSize) { return kInvalidJobSize; }
        return service_time * kAppParams.getServiceTimeScaleFactor(); // Scale
    }

public:
    virtual ~Application() {}
    DISALLOW_COPY_AND_ASSIGN(Application);

    /**
     * Returns the application type.
     */
    const std::string& type() const { return kType; }

    /**
     * Prints the application parameters.
     */
    void printConfiguration() const;

    /**
     * Returns whether this application requires flow ordering.
     */
    virtual bool isFlowOrderRequired() const = 0;

    /**
     * Processes the given network packet by invoking the application-
     * specific implementation and returns the actual job size (in ns).
     */
    virtual double process(const Packet& packet) = 0;

    /**
     * Returns the estimated time (in ns) to process the packet.
     */
    virtual double getJobSizeEstimate(const Packet& packet) = 0;
};

#endif // SIMULATOR_APPLICATIONS_APPLICATION_H
