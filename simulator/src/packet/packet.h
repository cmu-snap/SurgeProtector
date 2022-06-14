#ifndef SIMULATOR_PACKET_PACKET_H
#define SIMULATOR_PACKET_PACKET_H

// STD headers
#include <stdlib.h>
#include <stdexcept>

// Constant parameters
constexpr uint32_t kPacketSizeMinimumInBits = 512; // 64B
constexpr uint32_t kPacketSizeMaximumInBits = 12144; // 1518B

// Class of traffic (innocent or adversarial)
enum class TrafficClass { INNOCENT = 0, ATTACK };

/**
 * Represents a Flow ID.
 */
class FlowId final {
private:
    uint32_t src_ip_; // Source IP
    uint32_t dst_ip_; // Destination IP
    uint16_t src_port_; // Source port
    uint16_t dst_port_; // Destination port

public:
    explicit FlowId();
    explicit FlowId(const uint32_t s_ip, const uint32_t d_ip,
                    const uint16_t s_port, const uint16_t d_port);

    // Accessor methods
    uint32_t getSrcIP() const { return src_ip_; }
    uint32_t getDstIP() const { return dst_ip_; }
    uint16_t getSrcPort() const { return src_port_; }
    uint16_t getDstPort() const { return dst_port_; }

    // Ostream operator
    friend std::ostream&
    operator<<(std::ostream& out, const FlowId& id);

    /**
     * Helper method to construct a flow
     * ID for non-networking workloads.
     */
    static FlowId from(const uint32_t value);
};
// Sanity check
static_assert(sizeof(FlowId) == 12, "Bad FlowId packing");

/**
 * Recipe for hashing a flow ID.
 */
struct HashFlowId {
    size_t operator()(const FlowId& flow_id) const;
};

/**
 * Equality checking for flow IDs.
 */
struct EqualToFlowId {
    bool operator()(const FlowId& a, const FlowId& b) const;
};

/**
 * Represents TCP header data.
 */
class TCPHeader final {
private:
    bool is_valid_; // Valid header?

    bool flag_syn_; // TCP SYN flag
    bool flag_fin_; // TCP FIN flag
    bool flag_rst_; // TCP RST flag
    uint32_t psn_; // Packet sequence number
    uint32_t next_psn_; // Next expected PSN

public:
    explicit TCPHeader();
    explicit TCPHeader(
        const bool is_valid, const bool syn, const bool fin,
        const bool rst, const uint32_t psn, const uint32_t next_psn);

    // Accessors
    bool isValid() const { return is_valid_; }
    bool getFlagSyn() const { return flag_syn_; }
    uint32_t getSequenceNumber() const { return psn_; }
    uint32_t getNextSequenceNumber() const { return next_psn_; }
    bool isPassThroughPacket() const { return (psn_ == next_psn_); }
    bool isFlagFinOrRst() const { return (flag_fin_ || flag_rst_); }

    // Returns a pair representing the packet's PSN range: [start, end)
    std::pair<uint32_t, uint32_t> getSequenceNumberRange() const;
};

/**
 * Represents a network packet.
 */
class Packet final {
protected:
    uint64_t idx_;          // (Unique) packet index
    FlowId flow_id_;        // Corresponding flow ID
    TrafficClass class_;    // Packet traffic class
    uint32_t packet_size_;  // Packet size

    // TCP header data. TODO(natre): This should only
    // really exist in a version of the Packet class
    // that is specialized to TCP.
    TCPHeader tcp_header_;

    // Job sizes
    double job_size_actual_;
    double job_size_estimate_;

    // Housekeeping
    double arrive_time_ = 0; // Time of arrival
    double depart_time_ = 0; // Time of departure

public:
    explicit Packet();
    explicit Packet(const uint64_t idx, const FlowId flow_id,
                    const TrafficClass c, const uint32_t p);
    // Accessors
    uint64_t getPacketIdx() const { return idx_; }
    TrafficClass getClass() const { return class_; }
    const FlowId& getFlowId() const { return flow_id_; }
    double getArriveTime() const { return arrive_time_; }
    double getDepartTime() const { return depart_time_; }
    uint32_t getPacketSize() const { return packet_size_; }
    double getJobSizeActual() const { return job_size_actual_; }
    const TCPHeader& getTCPHeader() const { return tcp_header_; }
    double getJobSizeEstimate() const { return job_size_estimate_; }

    std::string getClassTag() const {
        return (class_ == TrafficClass::ATTACK) ? "A" : "I";
    }

    double getLatency() const {
        if (depart_time_ < arrive_time_) {
            throw std::runtime_error("Departure time must be GEQ arrival time");
        }
        return (depart_time_ - arrive_time_);
    }
    // Mutators
    void setDepartTime(const double time) { depart_time_ = time; }
    void setArriveTime(const double time) { arrive_time_ = time; }
    void setTCPHeader(const TCPHeader& header) { tcp_header_ = header; }
    void setJobSizeActual(const double jsize) { job_size_actual_ = jsize; }
    void setJobSizeEstimate(const double jsize) { job_size_estimate_ = jsize; }
};

#endif // SIMULATOR_PACKET_PACKET_H
