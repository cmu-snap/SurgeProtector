#ifndef SCHEDULER_HEAPS_FCFS_QUEUE_HPP
#define SCHEDULER_HEAPS_FCFS_QUEUE_HPP

// Library headers
#include "common/macros.h"
#include "common/utils.h"

// STD headers
#include <queue>
#include <stdexcept>

/**
 * Represents a basic FCFS queue.
 */
template<class Tag> class FCFSQueue {
private:
    std::queue<Tag> queue_;

public:
    /**
     * Returns the current queue size.
     */
    size_t size() const { return queue_.size(); }

    /**
     * Returns whether the queue is empty.
     */
    size_t empty() const { return queue_.empty(); }

    /**
     * Returns (w/o popping) the element tag at the front of the queue.
     * @throw runtime error if the queue is currently empty.
     */
    Tag peek() const {
        if (UNLIKELY(queue_.empty())) {
            throw std::runtime_error("Cannot peek an empty queue.");
        }
        return queue_.front();
    }

    /**
     * Pops (and returns) the element tag at the front of the queue.
     * @throw runtime error if the queue is currently empty.
     */
    Tag pop() {
        if (UNLIKELY(queue_.empty())) {
            throw std::runtime_error("Cannot pop an empty queue.");
        }
        Tag entry = queue_.front(); // Fetch the head entry
        queue_.pop(); // Pop the queue
        return entry;
    }

    /**
     * Pushes a new entry onto the queue.
     */
    void push(const Tag tag, const double weight) {
        SUPPRESS_UNUSED_WARNING(weight);
        queue_.push(tag);
    }
};

#endif // SCHEDULER_HEAPS_FCFS_QUEUE_HPP