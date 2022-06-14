#ifndef SIMULATOR_APPLICATIONS_TCP_REASSEMBLY_H
#define SIMULATOR_APPLICATIONS_TCP_REASSEMBLY_H

// Library headers
#include "application.h"

// STD headers
#include <list>
#include <stdlib.h>
#include <unordered_map>

/**
 * Represents per-flow TCP state.
 */
class TCPFlowState final {
private:
    typedef std::list<std::pair<uint32_t, uint32_t>> OOOList;
    typedef OOOList::const_iterator OOOListConstIter;

    // TODO(natre): Make these configurable parameters.
    static constexpr uint32_t kCostBase = 116;
    static constexpr uint32_t kCostPerTraversal = 4;
    static constexpr uint32_t kReassemblyWindowSizeInBytes = (1 << 16);

    // Housekeeping
    OOOList ooo_list_; // Out-of-order list
    uint32_t next_psn_; // Next sequence number

    /**
     * Internal helper method. Given the number of OOO linked-list
     * traversals to perform, returns the context-specific service
     * time for the corresponding packet.
     */
    static uint32_t toServiceTime(const uint32_t num_traversals);

    /**
     * Returns: 1) An iterator to the insertion position
     * in the OOO list corresponding to this packet, and
     * 2) The number of required linked list traversals.
     */
    std::pair<OOOListConstIter, uint32_t>
    getIteratorAfterInsertionPosition(const Packet& packet) const;

public:
    explicit TCPFlowState() : next_psn_(0) {}

    /**
     * Returns the expected service time for this packet.
     */
    double getServiceTimeEstimate(const Packet& packet) const;

    /**
     * Inserts the given packet into the appropriate position in the
     * OOO list, and returns the context-specific service time. Also
     * releases any segments that subsequently become in-order.
     */
    double process(const Packet& packet);
};

/**
 * Represents a TCP Reassembly engine.
 */
class TCPReassembly final : public Application {
private:
    std::unordered_map<FlowId, TCPFlowState,
        HashFlowId, EqualToFlowId> flows_; // Flow ID -> Flow state

    /**
     * Internal helper method. Processes the given network
     * packet and returns the job size (in ns).
     */
    double process_(const Packet& packet, bool update);

public:
    virtual ~TCPReassembly() {}
    explicit TCPReassembly(const Application::Parameters& p) :
                           Application(name(), p) {}
    /**
     * Returns the application name.
     */
    static std::string name() { return "tcp_reassembly"; }

    /**
     * Returns whether this application requires flow ordering.
     */
    virtual bool isFlowOrderRequired() const override { return true; }

    /**
     * Processes the given network packet by invoking the application-
     * specific implementation and returns the actual job size (in ns).
     */
    virtual double process(const Packet& packet) override;

    /**
     * Returns the expected job size (in ns) to process the packet.
     */
    virtual double getJobSizeEstimate(const Packet& packet) override;
};

#endif // SIMULATOR_APPLICATIONS_TCP_REASSEMBLY_H
