#ifndef SIMULATOR_QUEUEING_FQ_QUEUE_H
#define SIMULATOR_QUEUEING_FQ_QUEUE_H

// Library headers
#include "base_queue.h"
#include "common/utils.h"
#include "packet/packet.h"

// STD headers
#include <unordered_map>

// Boost headers
#include <boost/heap/binomial_heap.hpp>

/**
 * Per-flow metadata in a FQ-based queue.
 */
class FQFlowMetadata {
private:
    size_t num_packets_ = 0; // Number of queued packets
    double virtual_clock_ = 0; // Virtual clock for the tail packet

public:
    // Accessors
    size_t getNumPackets() const { return num_packets_; }
    double getVirtualClock() const { return virtual_clock_; }

    /**
     * Append a new packet to the flow queue.
     */
    FQFlowMetadata& push(const Packet& packet);

    /**
     * Deque the packet at the front of the flow queue.
     * @return Whether the flow queue becomes empty.
     */
    bool pop();
};

/**
 * Represents a FQ-based queue.
 */
class FQQueue : public BaseQueue {
private:
    size_t size_ = 0; // Queue size
    typedef MinHeapEntry<Packet, double> FQPriorityEntry;
    std::unordered_map<FlowId, FQFlowMetadata, HashFlowId,
                       EqualToFlowId> data_; // Flow ID -> Metadata
    boost::heap::binomial_heap<FQPriorityEntry> queue_; // Packet queue

public:
    explicit FQQueue() : BaseQueue(name()) {}
    virtual ~FQQueue() {}

    /**
     * Returns the policy name.
     */
    static std::string name() { return "fq"; }

    /**
     * Returns the number of packets in the queue.
     */
    virtual size_t size() const override { return size_; }

    /**
     * Returns whether the packet queue is empty.
     */
    virtual bool empty() const override { return (size_ == 0); }

    /**
     * Returns whether this queue maintains flow ordering.
     */
    virtual bool isFlowOrderMaintained() const override { return true; }

    /**
     * Pops (and returns) the packet at the front of the queue.
     * @throw runtime error if the queue is currently empty.
     */
    virtual Packet pop() override;

    /**
     * Returns (w/o popping) the packet at the front of the queue.
     * @throw runtime error if the queue is currently empty.
     */
    virtual Packet peek() const override;

    /**
     * Pushes a new packet onto the queue.
     */
    virtual void push(const Packet& packet) override;
};

#endif // SIMULATOR_QUEUEING_FQ_QUEUE_H
