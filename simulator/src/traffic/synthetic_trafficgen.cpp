#include "synthetic_trafficgen.h"

// Library headers
#include "common/utils.h"

// STD headers
#include <iomanip>
#include <iostream>

/**
 * SyntheticTrafficGenerator implementation.
 */
void SyntheticTrafficGenerator::reset() {
    arrival_time_ = 0;
    next_flow_id_ = 0;
}

void SyntheticTrafficGenerator::updateArrivalTime() {
    arrival_time_ += iat_dist_->sample();
}

Packet SyntheticTrafficGenerator::getNextArrival(const uint64_t packet_idx) {
    Packet packet = getNextArrivalImpl(packet_idx);
    packet.setArriveTime(arrival_time_);
    return packet;
}

/**
 * InnocentTrafficGenerator implementation.
 */
Packet InnocentTrafficGenerator::
getNextArrivalImpl(const uint64_t packet_idx) {
    // Generate the next (innocent) flow ID to use
    FlowId flow_id = FlowId::from(next_flow_id_);
    if (++next_flow_id_ == kNumFlows) {
        next_flow_id_ = 0;
    }
    // Generate a packet with this flow_id and packet size
    return Packet(packet_idx, flow_id, TrafficClass::INNOCENT,
                  static_cast<uint32_t>(psize_dist_->sample()));
}

double InnocentTrafficGenerator::getRateInBitsPerSecondImpl() const {
    // The rate is computed as E[P] / E[T], where P, T represent
    // packet and inter-arrival time distributions, respectively.
    return ((psize_dist_->getSampleStats().getMean() *
             kNanosecsPerSec) / iat_dist_->getSampleStats().getMean());
}

double InnocentTrafficGenerator::getAveragePacketSizeInBitsImpl() const {
    return psize_dist_->getSampleStats().getMean();
}

void InnocentTrafficGenerator::printConfiguration() const {
    std::cout << "{"
              << std::endl << "\ttype: " << name() << ","
              << std::endl << "\tiat_ns_dist: ";

    iat_dist_->printConfiguration();
    std::cout << "," << std::endl << "\tpacket_size_bits_dist: ";

    psize_dist_->printConfiguration();
    if (is_calibrated_) {
        std::cout << ","
                  << std::endl << "\trate: "
                  << getCalibratedRateInBitsPerSecond() << " bps";
    }
    std::cout << std::endl << "}" << std::endl;
}

void InnocentTrafficGenerator::calibrate(const double rate) {
    if (isCalibrated()) {
        throw std::runtime_error("Traffic-generator was already calibrated.");
    }
    else if (!std::isnan(rate)) {
        if (!DoubleApproxEqual(rate, getRateInBitsPerSecondImpl())) {
            throw std::runtime_error("Calibration failed, check computed rate.");
        }
        else { is_calibrated_ = true; }
    }
}

/**
 * AttackTrafficGenerator implementation.
 */
AttackTrafficGenerator::AttackTrafficGenerator(
    const uint32_t num_flows, const uint32_t fid_offset,
    ConstantDistribution* const iat_dist, const uint32_t p,
    const double j) : SyntheticTrafficGenerator(num_flows, iat_dist),
    kFlowIdOffset(fid_offset), kAttackJobSizeNs(j), kAttackPacketSizeBits(p) {

    // Special case: zero attack bandwidth (innocent arrivals only)
    if (iat_dist_->getSampleStats().getMean() == kDblPosInfty) {
        arrival_time_ = kDblPosInfty;
    }
}

Packet AttackTrafficGenerator::
getNextArrivalImpl(const uint64_t packet_idx) {
    // Compute the current attack flow ID and generate the next one
    uint32_t flow_id = (kFlowIdOffset + next_flow_id_);
    if (++next_flow_id_ == kNumFlows) {
        next_flow_id_ = 0;
    }
    Packet p = Packet(packet_idx, FlowId::from(flow_id),
                      TrafficClass::ATTACK, kAttackPacketSizeBits);

    // TODO(natre): Allow spoofing of job-size estimates
    p.setJobSizeEstimate(kAttackJobSizeNs);
    p.setJobSizeActual(kAttackJobSizeNs);
    return p;
}

double AttackTrafficGenerator::getRateInBitsPerSecondImpl() const {
    // The rate is computed as E[P] / E[T], where P, T represent
    // packet and inter-arrival time distributions, respectively.
    return ((kAttackPacketSizeBits * kNanosecsPerSec) /
            iat_dist_->getSampleStats().getMean());
}

double AttackTrafficGenerator::getAveragePacketSizeInBitsImpl() const {
    return kAttackPacketSizeBits;
}

void AttackTrafficGenerator::printConfiguration() const {
    std::cout << "{"
              << std::endl << "\ttype: " << name() << ","
              << std::endl << "\tiat_ns_dist: ";

    iat_dist_->printConfiguration();
    if (is_calibrated_) {
        std::cout << ","
                  << std::endl << "\tpacket_size_bits: "
                  << std::fixed << std::setprecision(2)
                  << kAttackPacketSizeBits << " bits,"
                  << std::endl << "\tjob_size_ns: "
                  << kAttackJobSizeNs << " ns,"
                  << std::endl << "\trate: "
                  << getCalibratedRateInBitsPerSecond() << " bps";
    }
    std::cout << std::endl << "}"
              << std::endl;
}

void AttackTrafficGenerator::calibrate(const double rate) {
    if (isCalibrated()) {
        throw std::runtime_error("Traffic-generator was already calibrated.");
    }
    else if (!std::isnan(rate)) {
        if (!DoubleApproxEqual(rate, getRateInBitsPerSecondImpl())) {
            throw std::runtime_error("Calibration failed, check computed rate.");
        }
        else { is_calibrated_ = true; }
    }
}
