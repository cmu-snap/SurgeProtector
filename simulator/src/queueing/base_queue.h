#ifndef SIMULATOR_QUEUEING_BASE_QUEUE_H
#define SIMULATOR_QUEUEING_BASE_QUEUE_H

// Library headers
#include "packet/packet.h"

// STD headers
#include <string>
#include <exception>

/**
 * Base class representing a generic queue.
 */
class BaseQueue {
protected:
    const std::string kType; // Underlying policy name
    explicit BaseQueue(const std::string type) : kType(type) {}

    // Internal helper method. Throws an error if
    // attempting to pop or peek an empty queue.
    static void assertNotEmpty(const bool empty) {
        if (empty) { throw std::runtime_error(
            "Cannot peek/pop an empty queue.");
        }
    }

public:
    virtual ~BaseQueue() {}

    /**
     * Returns the policy name.
     */
    const std::string& type() const { return kType; }

    /**
     * Returns the number of packets in the queue.
     */
    virtual size_t size() const = 0;

    /**
     * Returns whether the packet queue is empty.
     */
    virtual bool empty() const = 0;

    /**
     * Returns whether this queue maintains flow ordering.
     */
    virtual bool isFlowOrderMaintained() const = 0;

    /**
     * Pops (and returns) the packet at the front of the queue.
     * @throw runtime error if the queue is currently empty.
     */
    virtual Packet pop() = 0;

    /**
     * Returns (w/o popping) the packet at the front of the queue.
     * @throw runtime error if the queue is currently empty.
     */
    virtual Packet peek() const = 0;

    /**
     * Pushes a new packet onto the queue.
     */
    virtual void push(const Packet& packet) = 0;
};

#endif // SIMULATOR_QUEUEING_BASE_QUEUE_H
