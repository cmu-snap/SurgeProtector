#include "policy_fcfs.h"

PolicyFCFS::PolicyFCFS(struct rte_mempool* mbuf_pool,
                       struct rte_ring* process_ring) :
                       process_ring_(process_ring),
                       mbuf_pool_(mbuf_pool) {}

PolicyFCFS::~PolicyFCFS() {}

/**
 * Returns the name of this scheduling policy.
 */
std::string PolicyFCFS::name() { return "fcfs"; }

/**
 * Dequeues a burst of packets from the packet
 * queue and pushes them onto the process ring.
 */
void PolicyFCFS::scheduleBurst() {}

/**
 * Enqueues a burst of packets in to the RX packet queue.
 * If the queue becomes full, also deallocates the mbufs
 * corresponding to the dropped packets.
 *
 * @param mbufs Pointer to an array of mbufs to enqueue.
 * @param num_mbufs Number of mbufs to enqueue.
 */
void PolicyFCFS::enqueueBurst(struct rte_mbuf** mbufs,
                              const uint16_t num_mbufs) {
    const uint16_t num_enqueued = rte_ring_sp_enqueue_burst(
        process_ring_, (void**) mbufs, num_mbufs, NULL);

    for (auto idx = num_enqueued; idx < num_mbufs; idx++) {
        rte_pktmbuf_free(mbufs[idx]);
    }
    num_rx_ += num_enqueued;
}
