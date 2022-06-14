#ifndef SCHEDULER_BENCHMARK_SERVER_POLICIES_POLICY_WSJF_DROPMAX_H
#define SCHEDULER_BENCHMARK_SERVER_POLICIES_POLICY_WSJF_DROPMAX_H

// Library headers
#include "heaps/bounded_heap.hpp"

// STD headers
#include <stdint.h>
#include <string>

// DPDK headers
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_ring.h>

/**
 * Implements the WSJF scheduling policy using a pair of
 * Fibonacci heap, dropping packets corresponding to the
 * max heap entries.
 */
class PolicyWSJFFibonacciDropMax {
private:
    uint64_t num_rx_ = 0;
    BoundedHeap<rte_mbuf*> queue_;
    struct rte_ring* process_ring_;
    struct rte_mempool* mbuf_pool_;

public:
    static std::string name();
    PolicyWSJFFibonacciDropMax(struct rte_mempool* mbuf_pool,
                               struct rte_ring* process_ring);
    void scheduleBurst();
    void enqueueBurst(struct rte_mbuf** mbufs,
                      const uint16_t num_mbufs);
};

#endif // SCHEDULER_BENCHMARK_SERVER_POLICIES_POLICY_WSJF_DROPMAX_H
