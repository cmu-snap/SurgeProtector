#ifndef SIMULATOR_QUEUEING_SJF_QUEUE_H
#define SIMULATOR_QUEUEING_SJF_QUEUE_H

// Library headers
#include "base_queue.h"
#include "common/utils.h"
#include "packet/packet.h"

// Boost headers
#include <boost/heap/binomial_heap.hpp>

/**
 * Represents an SJF queue.
 */
class SJFQueue : public BaseQueue {
private:
    // Heap-based queue implementation
    typedef MinHeapEntry<Packet, double> SJFPriorityEntry;
    boost::heap::binomial_heap<SJFPriorityEntry> queue_;

public:
    explicit SJFQueue() : BaseQueue(name()) {}
    virtual ~SJFQueue() {}

    /**
     * Returns the policy name.
     */
    static std::string name() { return "sjf"; }

    /**
     * Returns the number of packets in the queue.
     */
    virtual size_t size() const override { return queue_.size(); }

    /**
     * Returns whether the packet queue is empty.
     */
    virtual bool empty() const override { return queue_.empty(); }

    /**
     * Returns whether this queue maintains flow ordering.
     */
    virtual bool isFlowOrderMaintained() const override { return false; }

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

#endif // SIMULATOR_QUEUEING_SJF_QUEUE_H
