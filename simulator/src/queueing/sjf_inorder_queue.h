#ifndef SIMULATOR_QUEUEING_SJF_INORDER_QUEUE_H
#define SIMULATOR_QUEUEING_SJF_INORDER_QUEUE_H

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
typedef MinHeapEntry<FlowId, double> SJFPriorityEntry;
typedef boost::heap::binomial_heap<SJFPriorityEntry>::handle_type handle_t;

/**
 * Per-flow metadata in an inorder SJF queue.
 */
class SJFInorderFlowMetadata {
private:
    handle_t handle_; // Heap entry handle
    std::queue<Packet> queue_; // Flow queue
    double total_jsize_ = 0; // Cumulative job size (numerator)

public:
    // Accessors
    handle_t getHandle() const { return handle_; }
    double getTotalJobSize() const { return total_jsize_; }
    double getFlowRatio() const { return (total_jsize_ / queue_.size()); }

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
    SJFInorderFlowMetadata& push(const Packet& packet);

    /**
     * Deque the packet at the front of the flow queue.
     */
    Packet pop();
};

/**
 * Represents a flow-based, in-order SJF queue that schedules
 * packets in increasing order of (Sigma(J_{i}) / n), where
 * J_{i} and n represent the job size of each queued packet
 * and the size of the queue for that flow, respectively.
 */
class SJFInorderQueue : public BaseQueue {
private:
    // Heap-based queue implementation
    size_t size_ = 0;
    boost::heap::binomial_heap<SJFPriorityEntry> priorities_;
    std::unordered_map<FlowId, SJFInorderFlowMetadata, HashFlowId,
                       EqualToFlowId> data_; // Flow ID -> Metadata
public:
    explicit SJFInorderQueue() : BaseQueue(name()) {}
    virtual ~SJFInorderQueue() {}

    /**
     * Returns the policy name.
     */
    static std::string name() { return "sjf_inorder"; }

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

#endif // SIMULATOR_QUEUEING_SJF_INORDER_QUEUE_H
