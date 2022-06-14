#include "iid_job_sizes.h"

// Library headers
#include "common/distributions/distribution_factory.h"

double IIDJobSizes::process(const Packet& packet) {
    // For attack traffic, use the packet-encoded job size
    if (packet.getClass() == TrafficClass::ATTACK) {
        assert(packet.getJobSizeActual() == packet.getJobSizeEstimate());
        assert(packet.getJobSizeActual() >= 0);
        return packet.getJobSizeActual();
    }
    else {
        // By this point, the packet's job size field must
        // be set. As such, we simply return this value.
        assert(packet.getJobSizeEstimate() >= 0);
        return packet.getJobSizeEstimate();
    }
}

double IIDJobSizes::getJobSizeEstimate(const Packet& packet) {
    // For attack traffic, use the packet-encoded job size
    if (packet.getClass() == TrafficClass::ATTACK) {
        assert(packet.getJobSizeEstimate() >= 0);
        return packet.getJobSizeEstimate();
    }
    // Otherwise, sample from the distribution
    else { return jsize_dist_->sample(); }
}
