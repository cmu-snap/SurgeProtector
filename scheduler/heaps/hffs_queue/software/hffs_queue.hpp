#ifndef SCHEDULER_HEAPS_HFFS_QUEUE_HPP
#define SCHEDULER_HEAPS_HFFS_QUEUE_HPP

// Library headers
#include "common/macros.h"
#include "common/utils.h"

// STD headers
#include <list>
#include <stdexcept>
#include <vector>

/**
 * Represents an approximate min-heap, implemented
 * using a Hierarchical Find First Set (FFS) Queue.
 */
template<class Tag, class Weight=double>
class HierarchicalFindFirstSetQueue {
private:
    /**
     * Memoized level state.
     */
    struct LevelState {
        uint32_t bitmap_idx = 0;
        uint32_t nonzero_bit_idx = 0;
    };

    // Queue parameters
    Weight kScaleFactor = 0;
    uint32_t kNumLevels = 0;
    uint32_t kNumBitmaps = 0;
    const uint32_t kNumBuckets = 0;

    // Housekeeping
    uint32_t size_ = 0;
    uint32_t* bitmaps_ = nullptr;
    std::vector<uint32_t> level_offsets_;
    std::vector<LevelState> level_stack_;
    std::vector<std::list<Tag>> buckets_;

    /**
     * Initializes the queue state.
     */
    void init() {
        kNumLevels = 1; kNumBitmaps = 1;
        uint32_t current_buckets = 32;
        level_offsets_.push_back(0);

        // Compute the number of levels and total number of
        // bitmaps required to represent the bucket queue.
        while (kNumBuckets > current_buckets) {
            level_offsets_.push_back(kNumBitmaps);
            kNumBitmaps += current_buckets;
            current_buckets *= 32;
            kNumLevels++;
        }
        // Allocate the buckets, bitmap tree, and level stack
        bitmaps_ = new uint32_t[kNumBitmaps]{};
        level_stack_.resize(kNumLevels);
        buckets_.resize(kNumBuckets);
    }

public:
    explicit HierarchicalFindFirstSetQueue(
        const uint32_t num_buckets, const Weight scale_factor) :
        kScaleFactor(scale_factor), kNumBuckets(num_buckets) {
        init(); // Init the queue state
    }
    ~HierarchicalFindFirstSetQueue() { delete[] bitmaps_; }

    /**
     * Unscaled weight parameters.
     */
    struct UnscaledWeight {
        Weight numerator;
        Weight denominator;
    };

    /**
     * Returns the current queue size.
     */
    size_t size() const { return size_; }

    /**
     * Returns whether the queue is empty.
     */
    size_t empty() const { return (size_ == 0); }

    /**
     * Internal helper method. Pops (and returns) the tag
     * corresponding to the {min, max} element in the queue.
     * @throw runtime error if the queue is currently empty.
     */
    template<bool IsPopMin>
    Tag pop_() {
        if (UNLIKELY(empty())) {
            throw std::runtime_error("Cannot pop an empty queue.");
        }
        uint32_t intralevel_bitmap_idx = 0;
        for (int idx = 0; idx < (int) kNumLevels; idx++) {
            uint32_t bitmap_idx = (level_offsets_[idx] +
                                   intralevel_bitmap_idx);
            // Sanity check
            SP_ASSERT(LIKELY(bitmaps_[bitmap_idx] != 0));

            // Memoize the bitmap and bit indices for this level
            int bit_idx;
            if (IsPopMin) {
                bit_idx = __builtin_ctz(bitmaps_[bitmap_idx]);
            }
            else {
                bit_idx = ((sizeof(*bitmaps_) * 8 - 1) -
                           __builtin_clz(bitmaps_[bitmap_idx]));
            }
            level_stack_[idx].nonzero_bit_idx = bit_idx;
            level_stack_[idx].bitmap_idx = bitmap_idx;

            // Update the intralevel bitmap index
            intralevel_bitmap_idx = (
                (intralevel_bitmap_idx * 32) + bit_idx);
        }
        // Pop the corresponding bucket
        uint32_t bucket_idx = intralevel_bitmap_idx; // Actual bucket
        SP_ASSERT(LIKELY(!buckets_[bucket_idx].empty()));
        Tag entry = buckets_[bucket_idx].front();
        buckets_[bucket_idx].pop_front();

        // If this bucket becomes empty, update the bitmap tree
        bool update = buckets_[bucket_idx].empty();
        for (int idx = (kNumLevels - 1); idx >= 0 && update; idx--) {
            uint32_t mask = ~(1UL << level_stack_[idx].nonzero_bit_idx);
            uint32_t bitmap_idx = level_stack_[idx].bitmap_idx;
            bitmaps_[bitmap_idx] &= mask; // Apply the mask
            update = (bitmaps_[bitmap_idx] == 0);
        }
        size_--; // Update queue size
        return entry;
    }

    /**
     * Pops (and returns) the tag corresponding to the min element.
     * @throw runtime error if the queue is currently empty.
     */
    inline Tag popMin() { return pop_<true>(); }

    /**
     * Pops (and returns) the tag corresponding to the max element.
     * @throw runtime error if the queue is currently empty.
     */
    inline Tag popMax() { return pop_<false>(); }

    /**
     * Pushes a new entry onto the queue.
     */
    void push(const Tag tag, const UnscaledWeight weight) {
        // Insert the given entry into the corresponding bucket
        uint32_t bucket_idx = uint32_t(
            (weight.numerator * kScaleFactor) / weight.denominator);

        // Sanity check
        SP_ASSERT(LIKELY(bucket_idx < kNumBuckets));
        bool update = buckets_[bucket_idx].empty();
        buckets_[bucket_idx].push_back(tag);

        // Update the bitmap sub-tree
        int bit_idx = (bucket_idx & 0x1F);
        uint32_t intralevel_bitmap_idx = (bucket_idx / 32);
        for (int idx = (kNumLevels - 1); idx >= 0 && update; idx--) {
            uint32_t bitmap_idx = (level_offsets_[idx] +
                                   intralevel_bitmap_idx);
            // Apply the bitmask
            update = (bitmaps_[bitmap_idx] == 0);
            uint32_t mask = (1UL << bit_idx);
            bitmaps_[bitmap_idx] |= mask;

            // Compute the indices for the next level
            bit_idx = (intralevel_bitmap_idx & 0x1F);
            intralevel_bitmap_idx /= 32;
        }
        // Update queue size
        size_++;
    }
};

#endif // SCHEDULER_HEAPS_HFFS_QUEUE_HPP
