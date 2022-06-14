#ifndef SIMULATOR_TRAFFIC_TRACE_TRAFFICGEN_H
#define SIMULATOR_TRAFFIC_TRACE_TRAFFICGEN_H

// Library headers
#include "common/distributions/constant_distribution.h"
#include "trafficgen.h"

// STD headers
#include <fstream>
#include <sstream>

/**
 * Represents a trace-driven generator for innocent traffic.
 */
class TraceTrafficGenerator final : public TrafficGenerator {
private:
    std::string trace_fp_; // Trace file path

    // TODO(natre): Currently, this is hardcoded to use constant
    // inter-arrival times (representing a fluid model). Ideally,
    // the distribution should be a user-configurable parameter.
    ConstantDistribution* iat_dist_ = nullptr; // IATs (ns)
    std::ifstream trace_ifs_; // Trace input filestream
    double avg_psize_ = NAN; // Average packet size (in bits)

    // Housekeeping
    std::string line_; // Scratchpad

    // Internal helper method
    void updateHasNewArrival();

    // Accessors
    virtual uint32_t getNumFlows() const override { return 0; }
    virtual double getRateInBitsPerSecondImpl() const override;
    virtual double getAveragePacketSizeInBitsImpl() const override;

public:
    virtual ~TraceTrafficGenerator();
    explicit TraceTrafficGenerator(const std::string& trace_fp,
                                   ConstantDistribution* const iat_dist);

    /**
     * Returns the traffic-gen name.
     */
    static std::string name() { return "trace"; }

    /**
     * Print the distribution configuration.
     */
    virtual void printConfiguration() const override;

    /**
     * Resets the traffic-generator to its initial state.
     */
    virtual void reset() override;

    /**
     * Invokes the implementation-specific virtual method to update the
     * arrival time. TODO(natre): This is a hack. Ideally, the traffic-
     * generator should update the next arrival time of its own accord;
     * however, in some instances, the arrival rate is predicated on
     * job size(s), which are not known at this point.
     */
    virtual void updateArrivalTime() override;

    /**
     * Returns the next packet arrival by invoking the implementation-
     * specific virtual method.
     */
    virtual Packet getNextArrival(const uint64_t packet_idx) override;

    /**
     * Returns whether the traffic-generator is calibrated.
     *
     * For some traffic-gens (eg, trace-driven), the total packet bit-
     * rate ultimately depends on the workload's average packet size.
     * Since this is only determined after running a full simulation,
     * the first simulation run must be a "dry-run" of the trace. We
     * say that the traffic-generator is "calibrated" if the average
     * packet size is already known (ie, this is not a dry-run).
     */
    virtual bool isCalibrated() const override;

    // Calibrate the traffic-generator
    void calibrate(const double avg_psize);
};

#endif // SIMULATOR_TRAFFIC_TRACE_TRAFFICGEN_H
