#include "policy_wsjf_hffs.h"

// Library headers
#include "benchmark/packet.h"
#include "scheduler.hpp"

// Queue configuration
constexpr uint32_t kNumTotalPriorityBuckets = (32 * 32 * 32 * 32);
constexpr uint32_t kMaxWeight = (
    (kAttackJSizeInNs + kAttackPSizeInBytes - 1) / kAttackPSizeInBytes);
constexpr uint32_t kScaleFactor = (kNumTotalPriorityBuckets / kMaxWeight);

/**
 * Returns the name of this scheduling policy.
 */
std::string PolicyWSJFFHierarchicalFFS::name() {
    return "wsjf_hffs";
}

PolicyWSJFFHierarchicalFFS::PolicyWSJFFHierarchicalFFS(
    struct rte_mempool* mbuf_pool, struct rte_ring* process_ring) :
    process_ring_(process_ring), mbuf_pool_(mbuf_pool),
    queue_(kNumTotalPriorityBuckets, kScaleFactor) {}

/**
 * Dequeues a burst of packets from the packet
 * queue and pushes them onto the process ring.
 */
void PolicyWSJFFHierarchicalFFS::scheduleBurst() {
    // Enque packets into the process ring until either the
    // packet queue becomes empty or the ring becomes full.
    int free_slots = rte_ring_free_count(process_ring_);
    while (!queue_.empty() && (free_slots > 0)) {
        rte_ring_enqueue(process_ring_, queue_.popMin());
        free_slots--;
    }
}

/**
 * Enqueues a burst of packets in to the RX packet queue.
 * If the queue becomes full, also deallocates the mbufs
 * corresponding to the dropped packets.
 *
 * @param mbufs Pointer to an array of mbufs to enqueue.
 * @param num_mbufs Number of mbufs to enqueue.
 */
void PolicyWSJFFHierarchicalFFS::enqueueBurst(
    struct rte_mbuf** mbufs, const uint16_t num_mbufs) {
    for (auto idx = 0; idx < num_mbufs; idx++) {
        auto p = getPacketParams(mbufs[idx]);
        auto weight = (HierarchicalFindFirstSetQueue<rte_mbuf*, uint32_t>::
                       UnscaledWeight{p.jsize_ns, p.psize_bytes});

        queue_.push(mbufs[idx], weight);
        num_rx_++;
    }
    // Deallocate the mbufs corresponding to dropped packets
    while (queue_.size() > SCHEDULER_QUEUE_SIZE) {
        rte_pktmbuf_free(queue_.popMax());
        num_rx_--;
    }
}
