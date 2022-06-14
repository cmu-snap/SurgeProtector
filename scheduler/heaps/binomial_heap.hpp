#ifndef SCHEDULER_HEAPS_BINOMIAL_HEAP_HPP
#define SCHEDULER_HEAPS_BINOMIAL_HEAP_HPP

// Library headers
#include "common/macros.h"
#include "common/utils.h"

// STD headers
#include <stdexcept>

// Boost headers
#include <boost/heap/binomial_heap.hpp>

/**
 * Implements a min-heap using a Binomial heap.
 */
template<class Tag> class BinomialHeap final {
private:
    // Typedefs
    using QueueEntry = MinHeapEntry<Tag, double>;
    boost::heap::binomial_heap<QueueEntry> queue_;

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
        return queue_.top().tag();
    }

    /**
     * Pops (and returns) the element tag at the front of the queue.
     * @throw runtime error if the queue is currently empty.
     */
    Tag pop() {
        if (UNLIKELY(queue_.empty())) {
            throw std::runtime_error("Cannot pop an empty queue.");
        }
        QueueEntry entry = queue_.top(); // Fetch the highest priority entry
        queue_.pop(); // Pop the queue
        return entry.tag();
    }

    /**
     * Pushes a new entry onto the queue.
     */
    void push(const Tag tag, const double weight) {
        QueueEntry queue_entry(tag, weight);
        queue_.push(queue_entry);
    }
};

#endif // SCHEDULER_HEAPS_BINOMIAL_HEAP_HPP