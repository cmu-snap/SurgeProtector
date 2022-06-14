#include "policy_wsjf_dropmax.h"

// Library headers
#include "benchmark/packet.h"
#include "scheduler.hpp"

/**
 * Returns the name of this scheduling policy.
 */
std::string PolicyWSJFFibonacciDropMax::name() {
    return "wsjf_drop_max";
}

PolicyWSJFFibonacciDropMax::PolicyWSJFFibonacciDropMax(
    struct rte_mempool* mbuf_pool, struct rte_ring* process_ring) :
    queue_(SCHEDULER_QUEUE_SIZE), process_ring_(process_ring),
    mbuf_pool_(mbuf_pool) {}

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
void PolicyWSJFFibonacciDropMax::scheduleBurst() {
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
void PolicyWSJFFibonacciDropMax::enqueueBurst(
    struct rte_mbuf** mbufs, const uint16_t num_mbufs) {
    for (uint16_t idx = 0; idx < num_mbufs; idx++) {
        rte_mbuf* dropped_entry = nullptr;

        auto weight = getPacketWeight(getPacketParams(mbufs[idx]));
        if (queue_.push(mbufs[idx], weight, dropped_entry)) {
            rte_pktmbuf_free(dropped_entry);
        }
        else { num_rx_++; }
    }
}
