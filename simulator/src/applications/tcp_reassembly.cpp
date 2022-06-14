#include "tcp_reassembly.h"

// STD headers
#include <assert.h>

/**
 * TCPFlowState implementation.
 */
typedef std::list<std::pair<uint32_t, uint32_t>> OOOList;
typedef OOOList::const_iterator OOOListConstIter;

uint32_t TCPFlowState::toServiceTime(const uint32_t num_traversals) {
    return kCostBase + (kCostPerTraversal * num_traversals);
}

std::pair<OOOListConstIter, uint32_t> TCPFlowState::
getIteratorAfterInsertionPosition(const Packet& packet) const {
    TCPHeader header = packet.getTCPHeader();
    uint32_t num_traversals = 0;

    auto iter = ooo_list_.begin();
    while (iter != ooo_list_.end()) {
        num_traversals++;

        // The end of the following PSN interval in the OOO list
        // is GEQ to the next expected PSN for the given packet.
        if (header.getNextSequenceNumber() <= iter->second) { break; }
        iter++;
    }
    return std::make_pair(iter, num_traversals);
}

double TCPFlowState::getServiceTimeEstimate(const Packet& packet) const {
    const TCPHeader& header = packet.getTCPHeader();
    auto range = header.getSequenceNumberRange();

    // SYN packet (or equivalent)
    if (header.getFlagSyn() || (next_psn_ == 0)) {
        return kInvalidJobSize;
    }
    // In-order flow, and this packet keeps it so
    else if (ooo_list_.empty() && (next_psn_ >= range.first)) {
        return kInvalidJobSize;
    }
    // If PSN is past the flow's TCP reassembly window, drop the packet
    else if (range.first > (next_psn_ + kReassemblyWindowSizeInBytes)) {
        return kInvalidJobSize;
    }
    // Obviously a duplicate packet
    else if (next_psn_ >= range.second) { return kInvalidJobSize; }

    // Out-of-order flow, approximate the expected
    // service time as the size of the OOO list.
    return static_cast<double>(
        toServiceTime(ooo_list_.size()));
}

double TCPFlowState::process(const Packet& packet) {
    const TCPHeader& header = packet.getTCPHeader();
    auto range = header.getSequenceNumberRange();

    // SYN packet (or equivalent)
    if (header.getFlagSyn() || (next_psn_ == 0)) {
        if (ooo_list_.empty()) { // Actually a SYN packet
            next_psn_ = header.getNextSequenceNumber();
        }
        // Sanity check: Duplicate SYN packet shouldn't increase the PSN
        else { assert(header.getNextSequenceNumber() <= next_psn_); }
        return kInvalidJobSize;
    }
    // The corresponding flow is in-order, and this packet keeps it so
    else if (ooo_list_.empty() && (next_psn_ >= range.first)) {
        next_psn_ = std::max(next_psn_, range.second);
        return kInvalidJobSize;
    }
    // If PSN is past the flow's TCP reassembly window, drop the packet
    else if (range.first > (next_psn_ + kReassemblyWindowSizeInBytes)) {
        return kInvalidJobSize;
    }
    // Obviously a duplicate packet
    else if (next_psn_ >= range.second) { return kInvalidJobSize; }

    auto pos = getIteratorAfterInsertionPosition(packet);
    uint32_t num_traversals = pos.second; // Traversals
    auto next_iter = pos.first; // Insertion location
    range.first = std::max(next_psn_, range.first);

    // Not inserting at the tail
    if (next_iter != ooo_list_.end()) {
        range.second = std::min(
            range.second, next_iter->first);
    }
    // Not inserting at the head
    if (next_iter != ooo_list_.begin()) {
        auto prev_iter = std::prev(next_iter);
        while (prev_iter != ooo_list_.begin() &&
               prev_iter->first >= range.first) {

            // Erase the previous node
            prev_iter = std::prev(
                ooo_list_.erase(prev_iter));
        }
        // Deleting the new head
        if (prev_iter->first >= range.first) {
            ooo_list_.erase(prev_iter);
        }
        // Update the range, if required
        else {
            range.first = std::max(
                range.first, prev_iter->second);
        }
    }
    // This packet has at least one new byte
    if (range.second > range.first) {
        // Insert this packet into the OOO list
        ooo_list_.insert(next_iter, range);

        // Finally, release any in-order nodes
        auto iter = ooo_list_.begin();
        while (iter != ooo_list_.end() &&
               next_psn_ == iter->first) {
            // Update the expected PSN
            next_psn_ = iter->second;
            iter = ooo_list_.erase(iter);

            // Update the traversal count
            num_traversals++;
        }
    }
    // Compute the service time
    return static_cast<double>(
        toServiceTime(num_traversals));
}

/**
 * TCPReassembly implementation.
 */
double TCPReassembly::process_(
    const Packet& packet, bool update) {
    double service_time = kInvalidJobSize;
    const TCPHeader& tcp_header = packet.getTCPHeader();

    // Only require non-trivial processing for TCP packets
    if (tcp_header.isValid()) {
        // FIN/RST flags are set, terminate the flow
        if (tcp_header.isFlagFinOrRst()) {
            if (update) {
                auto iter = flows_.find(packet.getFlowId());
                if (iter != flows_.end()) { flows_.erase(iter); }
            }
        }
        // Else, process innocent traffic as usual
        else if (!tcp_header.isPassThroughPacket()) {
            // Find or insert the corresponding flow mapping
            const FlowId& flow_id = packet.getFlowId();
            auto iter = flows_.find(flow_id);
            if (update) {
                if (iter == flows_.end()) {
                    iter = flows_.insert(
                        {flow_id, TCPFlowState()}).first;
                }
                service_time = iter->second.process(packet);
            }
            // Flow exists, fetch its service time
            else if (iter != flows_.end()) {
                service_time = (iter->second.
                    getServiceTimeEstimate(packet));
            }
        }
    }
    return toJobSizeInNs(service_time);
}

double TCPReassembly::process(const Packet& packet) {
    // For attack traffic, use the packet-encoded job size
    if (packet.getClass() == TrafficClass::ATTACK) {
        assert(packet.getJobSizeActual() >= 0);
        return packet.getJobSizeActual();
    }
    // If using the heuristic, process the packet
    else if (kAppParams.getUseHeuristic()) {
        return process_(packet, true);
    }
    // Else, processing is performed during job-size estimation;
    // there is nothing to do. Simply yields packet's job size.
    else {
        return packet.getJobSizeEstimate();
    }
}

double TCPReassembly::getJobSizeEstimate(const Packet& packet) {
    // For attack traffic, use the packet-encoded job size
    if (packet.getClass() == TrafficClass::ATTACK) {
        assert(packet.getJobSizeEstimate() >= 0);
        return packet.getJobSizeEstimate();
    }
    // Important note: In TCP reassembly, the only way to precisely
    // determine the size of a job is to serve all packets from the
    // same flow that appear before it. In order to compute the job
    // size, we preemptively process a job when getJobSizeEstimate()
    // is invoked. Since the underlying queue guarantees in-order
    // service for same-flow packets, the TCP state remains valid.
    assert(packet.getJobSizeEstimate() == kInvalidJobSize);
    return process_(packet, !kAppParams.getUseHeuristic());
}
