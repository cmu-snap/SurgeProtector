#ifndef SIMULATOR_QUEUEING_FCFS_QUEUE_H
#define SIMULATOR_QUEUEING_FCFS_QUEUE_H

// Library headers
#include "base_queue.h"
#include "packet/packet.h"

// STD headers
#include <list>

/**
 * Represents an FCFS queue.
 */
class FCFSQueue : public BaseQueue {
private:
    std::list<Packet> queue_; // Packet queue

public:
    explicit FCFSQueue() : BaseQueue(name()) {}
    virtual ~FCFSQueue() {}

    /**
     * Returns the policy name.
     */
    static std::string name() { return "fcfs"; }

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

#endif // SIMULATOR_QUEUEING_FCFS_QUEUE_H
