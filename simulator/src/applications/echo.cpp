#include "echo.h"

// STD headers
#include <assert.h>

double Echo::process(const Packet& packet) {
    // For attack traffic, use the packet-encoded job size
    if (packet.getClass() == TrafficClass::ATTACK) {
        assert(packet.getJobSizeActual() >= 0);
        return packet.getJobSizeActual();
    }
    else {
        // By this point, the packet's job size field must
        // be set. As such, we simply return this value.
        return packet.getJobSizeEstimate();
    }
}

double Echo::getJobSizeEstimate(const Packet& packet) {
    // Use the packet-encoded job size estimate
    assert(packet.getJobSizeEstimate() >= 0);
    return packet.getJobSizeEstimate();
}
