#ifndef SCHEDULER_HEAPS_BOUNDED_HEAP_HPP
#define SCHEDULER_HEAPS_BOUNDED_HEAP_HPP

// Library headers
#include "common/macros.h"

// STD headers
#include <stdexcept>

// Boost headers
#include <boost/heap/fibonacci_heap.hpp>

/**
 * Represents a heap entry.
 */
template<bool IsMin, class Tag, class Metric>
class BoundedHeapEntry {
private:
    // Typedefs. TODO(natre): This is a hack. Figure out
    // how to templatize handle_t types in a generic way.
    using handle_t = typename boost::heap::fibonacci_heap
        <BoundedHeapEntry<!IsMin, Tag, Metric>>::handle_type;

    Tag tag_; // Tag corresponding to this entry
    Metric primary_metric_; // The primary priority metric
    handle_t handle_; // Housekeeping: handle to a heap entry

public:
    explicit BoundedHeapEntry(const Tag& tag, const Metric& metric) :
                              tag_(tag), primary_metric_(metric), handle_() {}
    // Accessors
    const Tag& tag() const { return tag_; }
    handle_t getHandle() const { return handle_; }
    Metric getPrimaryMetric() const { return primary_metric_; }

    // Mutators
    void setHandle(handle_t handle) { handle_ = handle; }

    // Comparator
    bool operator<(const BoundedHeapEntry& other) const {
        if (IsMin) {
            return primary_metric_ > other.primary_metric_;
        }
        else {
            return primary_metric_ < other.primary_metric_;
        }
    }
};

/**
 * Implements a bounded min-heap using a pair of Fibonacci heaps.
 */
template<class Tag> class BoundedHeap {
private:
    // Typedefs
    using QueueEntry = BoundedHeapEntry<true, Tag, double>;
    using DropQueueEntry = BoundedHeapEntry<false, Tag, double>;

    // Maximum queue size
    const size_t kMaxQueueSize;

    // Heap-based queue implementations. We maintain two queues:
    // "queue_" which implements the actual WSJF queue and whose
    // head represents the element with the lowest job-to-packet
    // size ratio, and "drop_queue_", whose head represents the
    // lowest-priority element to drop next (if required). This
    // is implemented using a max-heap, with the invariant that
    // the heaps are consistent (have the same size and set of
    // entries) and have a mutually opposite ordering.
    boost::heap::fibonacci_heap<QueueEntry> queue_;
    boost::heap::fibonacci_heap<DropQueueEntry> drop_queue_;

public:
    explicit BoundedHeap(const size_t max_size) :
                         kMaxQueueSize(max_size) {}
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
        drop_queue_.erase(entry.getHandle()); // Update the drop queue
        queue_.pop(); // Pop the queue

        SP_ASSERT(LIKELY(drop_queue_.size() == queue_.size()));
        return entry.tag();
    }

    /**
     * Pushes a new entry onto the queue. If the queue is at capacity,
     * also removes and returns the lowest-priority entry. Note: This
     * may be the parameterized entry itself.
     */
    bool push(const Tag tag, const double weight, Tag& erased_tag) {
        // Instantiate the queue entries.
        QueueEntry queue_entry(tag, weight);
        DropQueueEntry drop_queue_entry(tag, weight);

        // Push them onto the respective queues
        auto queue_handle = queue_.push(queue_entry);
        auto drop_queue_handle = drop_queue_.push(drop_queue_entry);

        // Update the handles
        (*queue_handle).setHandle(drop_queue_handle);
        (*drop_queue_handle).setHandle(queue_handle);

        // If required, pop the lowest-priority element
        if (queue_.size() > kMaxQueueSize) {
            DropQueueEntry erased_entry = drop_queue_.top();
            queue_.erase(erased_entry.getHandle());
            drop_queue_.pop();

            // Update the return value (erased entry tag)
            erased_tag = erased_entry.tag();
            return true;
        }
        SP_ASSERT(LIKELY(drop_queue_.size() == queue_.size()));
        return false;
    }
};

#endif // SCHEDULER_HEAPS_BOUNDED_HEAP_HPP
