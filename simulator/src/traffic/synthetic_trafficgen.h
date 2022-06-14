#ifndef SIMULATOR_TRAFFIC_SYNTHETIC_TRAFFICGEN_H
#define SIMULATOR_TRAFFIC_SYNTHETIC_TRAFFICGEN_H

// Library headers
#include "common/utils.h"
#include "common/distributions/constant_distribution.h"
#include "common/distributions/distribution.h"
#include "packet/packet.h"
#include "trafficgen.h"

/**
 * Base Class representing a synthetic traffic generator.
 */
class SyntheticTrafficGenerator : public TrafficGenerator {
protected:
    // TODO(natre): Currently, this is hardcoded to use constant
    // inter-arrival times (representing a fluid model). Ideally,
    // the distribution should be a user-configurable parameter.
    ConstantDistribution* iat_dist_ = nullptr; // IATs (ns)
    const uint32_t kNumFlows = 1; // Number of flows to use

    // Housekeeping
    uint32_t next_flow_id_ = 0;
    bool is_calibrated_ = false; // Calibrated?

    explicit SyntheticTrafficGenerator(
        const uint32_t num_flows, ConstantDistribution* const iat_dist) :
        TrafficGenerator(name()), iat_dist_(iat_dist), kNumFlows(num_flows) {}

    /**
     * Helper method. Returns the next packet arrival.
     */
    virtual Packet getNextArrivalImpl(const uint64_t packet_idx) = 0;

public:
    virtual ~SyntheticTrafficGenerator() { delete(iat_dist_); }

    /**
     * Returns the traffic-gen name.
     */
    static std::string name() { return "synthetic"; }

    // Accessors
    virtual uint32_t getNumFlows() const override { return kNumFlows; }

    /**
     * Resets the traffic-generator to its initial state.
     */
    virtual void reset() override;

    /**
     * Invokes the implementation-specific virtual method to update the
     * arrival time. TODO(natre): This is a hack. Ideally, the traffic-
     * generator should update the next arrival time of its own accord;
     * however, in some instances, the arrival rate is predicated on a
     * packet's true job size, which may not be known at this point.
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
    virtual bool isCalibrated() const override { return is_calibrated_; }
};

/**
 * Represents a traffic generator that generates packets with sizes
 * chosen from a user-specified distribution. This corresponds to
 * the innocent traffic workload.
 */
class InnocentTrafficGenerator final : public SyntheticTrafficGenerator {
private:
    // Packet size distribution
    Distribution* psize_dist_ = nullptr;

    /**
     * Returns the next arrival and updates the arrival clock.
     */
    virtual Packet getNextArrivalImpl(const uint64_t packet_idx) override;

    // Accessors
    virtual double getRateInBitsPerSecondImpl() const override;
    virtual double getAveragePacketSizeInBitsImpl() const override;

public:
    virtual ~InnocentTrafficGenerator() { delete(psize_dist_); }
    explicit InnocentTrafficGenerator(const uint32_t num_flows,
        ConstantDistribution* const iat_dist, Distribution* const psize_dist) :
        SyntheticTrafficGenerator(num_flows, iat_dist), psize_dist_(psize_dist) {}

    /**
     * Print the distribution configuration.
     */
    virtual void printConfiguration() const override;

    // Calibrate the traffic-generator
    void calibrate(const double rate);
};

/**
 * Represents an adversarial traffic generator.
 */
class AttackTrafficGenerator final : public SyntheticTrafficGenerator {
private:
    const uint32_t kFlowIdOffset; // Flow ID offset
    const double kAttackJobSizeNs; // Adversarial job size (ns)
    const uint32_t kAttackPacketSizeBits; // Adversarial packet size (bits)

    /**
     * Returns the next arrival and updates the arrival clock.
     */
    virtual Packet getNextArrivalImpl(const uint64_t packet_idx) override;

    // Accessors
    virtual double getRateInBitsPerSecondImpl() const override;
    virtual double getAveragePacketSizeInBitsImpl() const override;

public:
    explicit AttackTrafficGenerator();
    explicit AttackTrafficGenerator(const uint32_t num_flows,
        const uint32_t fid_offset, ConstantDistribution* const
        iat_dist, const uint32_t psize_bits, const double jsize_ns);

    virtual ~AttackTrafficGenerator() {}

    /**
     * Print the distribution configuration.
     */
    virtual void printConfiguration() const override;

    // Calibrate the traffic-generator
    void calibrate(const double rate);
};

#endif // SIMULATOR_TRAFFIC_SYNTHETIC_TRAFFICGEN_H
