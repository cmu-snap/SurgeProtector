#ifndef SIMULATOR_TRAFFIC_TRAFFICGEN_H
#define SIMULATOR_TRAFFIC_TRAFFICGEN_H

// Library headers
#include "packet/packet.h"

// STD headers
#include <string>
#include <vector>

/**
 * Base class representing a traffic generator.
 */
class TrafficGenerator {
protected:
    const std::string kType; // Traffic-gen type
    bool has_new_arrival_ = true; // Can generate more arrivals?
    double arrival_time_ = 0; // Arrival time for the next packet

    // Accessors
    virtual double getRateInBitsPerSecondImpl() const = 0;
    virtual double getAveragePacketSizeInBitsImpl() const = 0;

public:
    explicit TrafficGenerator(const std::string type) : kType(type) {}
    virtual ~TrafficGenerator() {}

    // Accessors
    virtual uint32_t getNumFlows() const = 0;
    double getCalibratedRateInBitsPerSecond() const;
    double getCalibratedAveragePacketSizeInBits() const;
    bool hasNewArrival() const { return has_new_arrival_; }
    double getNextArrivalTime() const { return arrival_time_; }

    /**
     * Returns the traffic-generator type.
     */
    const std::string& type() const { return kType; }

    /**
     * Print the distribution configuration.
     */
    virtual void printConfiguration() const = 0;

    /**
     * Resets the traffic-generator to its initial state.
     */
    virtual void reset() = 0;

    /**
     * Invokes the implementation-specific virtual method to update the
     * arrival time. TODO(natre): This is a hack. Ideally, the traffic-
     * generator should update the next arrival time of its own accord;
     * however, in some instances, the arrival rate is predicated on
     * job size(s), which are not known at this point.
     */
    virtual void updateArrivalTime() = 0;

    /**
     * Returns the next packet arrival by invoking the implementation-
     * specific virtual method.
     */
    virtual Packet getNextArrival(const uint64_t packet_idx) = 0;

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
    virtual bool isCalibrated() const = 0;
};

#endif // SIMULATOR_TRAFFIC_TRAFFICGEN_H
