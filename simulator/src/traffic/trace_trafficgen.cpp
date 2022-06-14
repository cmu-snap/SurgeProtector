#include "trace_trafficgen.h"

// Library headers
#include "common/macros.h"

// STD headers
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>

TraceTrafficGenerator::TraceTrafficGenerator(const std::string& trace_fp,
    ConstantDistribution* const iat_dist) : TrafficGenerator(name()),
    trace_fp_(trace_fp), iat_dist_(iat_dist) {
    // Initialize the trace filestream
    trace_ifs_.open(trace_fp);
    updateHasNewArrival();
}

TraceTrafficGenerator::~TraceTrafficGenerator() {
    // Deallocate resources
    trace_ifs_.close();
    delete(iat_dist_);
}

void TraceTrafficGenerator::updateHasNewArrival() {
    has_new_arrival_ = (trace_ifs_.peek() !=
                        std::ifstream::traits_type::eof());
}

double TraceTrafficGenerator::getRateInBitsPerSecondImpl() const {
    // The rate is computed as E[P] / E[T], where P, T represent
    // packet and inter-arrival time distributions, respectively.
    return ((avg_psize_ * kNanosecsPerSec) /
            iat_dist_->getSampleStats().getMean());
}

double TraceTrafficGenerator::
getAveragePacketSizeInBitsImpl() const { return avg_psize_; }

void TraceTrafficGenerator::printConfiguration() const {
    std::cout << "{"
              << std::endl << "\ttype: " << name() << ","
              << std::endl << "\ttrace: " << trace_fp_ << ","
              << std::endl << "\tiat_ns_dist: ";

    iat_dist_->printConfiguration();
    if (isCalibrated()) {
        std::cout << ","
                  << std::endl << "\taverage_packet_size_bits: "
                  << std::fixed << std::setprecision(2)
                  << avg_psize_ << " bits,"
                  << std::endl << "\trate: "
                  << getCalibratedRateInBitsPerSecond() << " bps";
    }
    std::cout << std::endl << "}" << std::endl;
}

void TraceTrafficGenerator::reset() {
    arrival_time_ = 0;
    trace_ifs_.clear();
    trace_ifs_.seekg(0);
    updateHasNewArrival();
}

void TraceTrafficGenerator::updateArrivalTime() {
    arrival_time_ += iat_dist_->sample();
}

Packet TraceTrafficGenerator::getNextArrival(const uint64_t packet_idx) {
    assert(has_new_arrival_); // Sanity check
    std::getline(trace_ifs_, line_);

    // Parse the packet fields
    auto values = split(line_, ",");
    assert(values.size() >= 9); // Minimum values

    // Extract Ethernet and IP fields
    uint32_t psize = (std::stoul(values[0], nullptr, 10) *
                      kBitsPerByte); // Ethernet packet size

    uint32_t src_ip = std::stoul(values[1], nullptr, 16); // Src IP
    uint32_t dst_ip = std::stoul(values[2], nullptr, 16); // Dst IP
    uint16_t src_port = std::stoul(values[3], nullptr, 16); // Src port
    uint16_t dst_port = std::stoul(values[4], nullptr, 16); // Dst port
    bool is_tcp = (std::stoi(values[5], nullptr, 10) == 1); // TCP packet?

    // Create the packet
    FlowId flow_id(src_ip, dst_ip, src_port, dst_port); // Flow ID
    Packet packet(packet_idx, flow_id, TrafficClass::INNOCENT, psize);

    // Configure the TCP header, if required
    if (is_tcp) {
        int flags = std::stoi(values[6], nullptr, 10); // TCP flags
        uint32_t psn = std::stoul(values[7], nullptr, 10); // TCP PSN
        uint32_t next_psn = std::stoul(values[8], nullptr, 10); // Next PSN

        packet.setTCPHeader(TCPHeader(
            true,                       // Valid header?
            ((flags >> 2) & 0x1) == 1,  // SYN flag
            ((flags >> 1) & 0x1) == 1,  // FIN flag
            (flags & 0x1) == 1,         // RST flag
            psn,                        // PSN
            next_psn                    // Next PSN
        ));
    }
    // If a job size is specified in the trace, use that
    if (values.size() > 9 && values[9] != "") {
        double jsize = std::stod(values[9]);
        packet.setJobSizeEstimate(jsize);
    }
    // End of trace?
    updateHasNewArrival();

    // Update the packet's arrival time and return
    packet.setArriveTime(arrival_time_);
    return packet;
}

bool TraceTrafficGenerator::isCalibrated() const {
    return !std::isnan(avg_psize_);
}

void TraceTrafficGenerator::calibrate(const double avg_psize) {
    if (isCalibrated()) {
        throw std::runtime_error("Traffic-generator was already calibrated.");
    }
    avg_psize_ = avg_psize;
}
