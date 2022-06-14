#include "packet.h"

// Library headers
#include "common/macros.h"

// STD headers
#include <iomanip>

// Boost headers
#include <boost/container_hash/hash.hpp>

/**
 * FlowId implementation.
 */
FlowId::FlowId() : FlowId(0, 0, 0, 0) {}
FlowId::FlowId(const uint32_t s_ip, const uint32_t d_ip,
               const uint16_t s_port, const uint16_t d_port) :
               src_ip_(s_ip), dst_ip_(d_ip), src_port_(s_port),
               dst_port_(d_port) {}

std::ostream&
operator<<(std::ostream& out, const FlowId& id) {
    out << std::hex << std::setfill('0')
        << std::setw(8) << id.src_ip_
        << std::setw(8) << id.dst_ip_
        << std::setw(4) << id.src_port_
        << std::setw(4) << id.dst_port_
        << std::dec;
    return out;
}

FlowId FlowId::from(const uint32_t value) {
    return FlowId(value, 0, 0, 0);
}

size_t HashFlowId::operator()(
    const FlowId& flow_id) const {
    std::size_t res = 0;
    boost::hash_combine(res, flow_id.getSrcIP());
    boost::hash_combine(res, flow_id.getDstIP());
    boost::hash_combine(res, flow_id.getSrcPort());
    boost::hash_combine(res, flow_id.getDstPort());
    return res;
}

bool EqualToFlowId::operator()(
    const FlowId& a, const FlowId& b) const {
    return ((a.getSrcIP() == b.getSrcIP())      &&
            (a.getDstIP() == b.getDstIP())      &&
            (a.getSrcPort() == b.getSrcPort())  &&
            (a.getDstPort() == b.getDstPort())  );
}

/**
 * TCPHeader implementation.
 */
TCPHeader::TCPHeader() : TCPHeader(
    false, false, false, false, 0, 0) {}

TCPHeader::TCPHeader(
    const bool is_valid, const bool syn, const bool fin, const
    bool rst, const uint32_t psn, const uint32_t next_psn) :
    is_valid_(is_valid), flag_syn_(syn), flag_fin_(fin),
    flag_rst_(rst), psn_(psn), next_psn_(next_psn) {}

std::pair<uint32_t, uint32_t>
TCPHeader::getSequenceNumberRange() const {
    return std::make_pair(psn_, next_psn_);
}

/**
 * Packet implementation.
 */
Packet::Packet() : Packet(
    0, FlowId(), TrafficClass::INNOCENT, 0) {}

Packet::Packet(const uint64_t idx, const FlowId flow_id,
               const TrafficClass c, const uint32_t p) :
               idx_(idx), flow_id_(flow_id), class_(c),
               packet_size_(p), tcp_header_(),
               job_size_actual_(kInvalidJobSize),
               job_size_estimate_(kInvalidJobSize) {}
