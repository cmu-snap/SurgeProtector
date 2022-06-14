#include "trafficgen.h"

// STD headers
#include <stdexcept>

/**
 * TrafficGenerator implementation.
 */
double TrafficGenerator::getCalibratedRateInBitsPerSecond() const {
    if (!isCalibrated()) {
        throw std::runtime_error("TrafficGenerator is not calibrated");
    }
    return getRateInBitsPerSecondImpl();
}

double TrafficGenerator::getCalibratedAveragePacketSizeInBits() const {
    if (!isCalibrated()) {
        throw std::runtime_error("TrafficGenerator is not calibrated");
    }
    return getAveragePacketSizeInBitsImpl();
}
