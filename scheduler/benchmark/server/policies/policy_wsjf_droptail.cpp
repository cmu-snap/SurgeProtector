#include "policy_wsjf_droptail.h"

// Library headers
#include "benchmark/packet.h"
#include "scheduler.hpp"

/**
 * Returns the name of this scheduling policy.
 */
std::string PolicyWSJFFibonacciDropTail::name() {
    return "wsjf_drop_tail";
}

PolicyWSJFFibonacciDropTail::PolicyWSJFFibonacciDropTail(
    struct rte_mempool* mbuf_pool, struct rte_ring* process_ring) :
    process_ring_(process_ring), mbuf_pool_(mbuf_pool), queue_() {}

/**
 * Computes and returns the packet weight.
 */
inline double getPacketWeight(const PacketParams params) {
    return ((double) params.jsize_ns) / params.psize_bytes;
}

/**
 * Dequeues a burst of packets from the packet
 * queue and pushes them onto the process ring.
 */
void PolicyWSJFFibonacciDropTail::scheduleBurst() {
    // Enque packets into the process ring until either the
    // packet queue becomes empty or the ring becomes full.
    int free_slots = rte_ring_free_count(process_ring_);
    while (!queue_.empty() && (free_slots > 0)) {
        rte_ring_enqueue(process_ring_, queue_.pop());
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
void PolicyWSJFFibonacciDropTail::enqueueBurst(
    struct rte_mbuf** mbufs, const uint16_t num_mbufs) {
    uint16_t num_enqueued = 0;
    while ((num_enqueued < num_mbufs) &&
           (queue_.size() < SCHEDULER_QUEUE_SIZE)) {
        // Insert the current mbuf into the packet queue
        auto params = getPacketParams(mbufs[num_enqueued]);
        queue_.push(mbufs[num_enqueued], getPacketWeight(params));

        // Increment the enque count
        num_enqueued++;
    }
    // Deallocate the mbufs corresponding to dropped packets
    for (auto idx = num_enqueued; idx < num_mbufs; idx++) {
        rte_pktmbuf_free(mbufs[idx]);
    }
    num_rx_ += num_enqueued;
}
