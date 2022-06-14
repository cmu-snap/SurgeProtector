#ifndef SIMULATOR_APPLICATIONS_IID_JOB_SIZES_H
#define SIMULATOR_APPLICATIONS_IID_JOB_SIZES_H

// Library headers
#include "application.h"
#include "common/distributions/distribution.h"

/**
 * Example application that picks job sizes for innocent
 * traffic i.i.d. from a user-specified distribution.
 */
class IIDJobSizes final : public Application {
private:
    // Job size distribution
    Distribution* jsize_dist_ = nullptr;

public:
    explicit IIDJobSizes(const Application::Parameters& params,
                         Distribution* const jsize_dist) :
                         Application(name(), params),
                         jsize_dist_(jsize_dist) {}

    virtual ~IIDJobSizes() { delete(jsize_dist_); }

    /**
     * Returns the application name.
     */
    static std::string name() { return "iid_job_sizes"; };

    /**
     * Returns whether this application requires flow ordering.
     */
    virtual bool isFlowOrderRequired() const override { return false; }

    /**
     * Processes the given network packet by invoking the application-
     * specific implementation and returns the actual job size (in ns).
     */
    virtual double process(const Packet& packet) override;

    /**
     * Returns the estimated time (in ns) to process the packet.
     */
    virtual double getJobSizeEstimate(const Packet& packet) override;
};

#endif // SIMULATOR_APPLICATIONS_IID_JOB_SIZES_H
