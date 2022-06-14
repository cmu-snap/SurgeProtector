#ifndef SIMULATOR_QUEUEING_WSJF_INORDER_QUEUE_H
#define SIMULATOR_QUEUEING_WSJF_INORDER_QUEUE_H

// Library headers
#include "base_queue.h"
#include "common/utils.h"
#include "packet/packet.h"

// STD headers
#include <queue>
#include <unordered_map>

// Boost headers
#include <boost/heap/binomial_heap.hpp>

// Typedefs
typedef MinHeapEntry<FlowId, double> WSJFPriorityEntry;
typedef boost::heap::binomial_heap<WSJFPriorityEntry>::handle_type handle_t;

/**
 * Per-flow metadata in an inorder WSJF queue.
 */
class WSJFInorderFlowMetadata {
private:
    handle_t handle_; // Heap entry handle
    std::queue<Packet> queue_; // Flow queue
    double total_jsize_ = 0; // Cumulative job size (numerator)
    uint64_t total_psize_ = 0; // Cumulative packet size (denominator)

public:
    // Accessors
    handle_t getHandle() const { return handle_; }
    double getTotalJobSize() const { return total_jsize_; }
    uint64_t getTotalPacketSize() const { return total_psize_; }
    double getFlowRatio() const { return (total_jsize_ / total_psize_); }

    // Mutators
    void setHandle(const handle_t handle) { handle_ = handle; }

    /**
     * Flow queue operations.
     */
    size_t size() const { return queue_.size(); }
    bool empty() const { return queue_.empty(); }
    Packet front() const { return queue_.front(); }

    /**
     * Append a new packet to the flow queue.
     */
    WSJFInorderFlowMetadata& push(const Packet& packet);

    /**
     * Deque the packet at the front of the flow queue.
     */
    Packet pop();
};

/**
 * Represents a flow-based, in-order Weighted SJF queue that schedules
 * packets in increasing order of (Sigma(J_{i}) / Sigma(P_{i})), where
 * J_{q} and P_{q} represent the job and packet sizes of the queued
 * entries in each flow, respectively.
 */
class WSJFInorderQueue : public BaseQueue {
private:
    // Heap-based queue implementation
    size_t size_ = 0;
    boost::heap::binomial_heap<WSJFPriorityEntry> priorities_;
    std::unordered_map<FlowId, WSJFInorderFlowMetadata, HashFlowId,
                       EqualToFlowId> data_; // Flow ID -> Metadata

public:
    explicit WSJFInorderQueue() : BaseQueue(name()) {}
    virtual ~WSJFInorderQueue() {}

    /**
     * Returns the policy name.
     */
    static std::string name() { return "wsjf_inorder"; }

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

#endif // SIMULATOR_QUEUEING_WSJF_INORDER_QUEUE_H
