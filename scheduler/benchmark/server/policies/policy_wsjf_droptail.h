#ifndef SCHEDULER_BENCHMARK_SERVER_POLICIES_POLICY_WSJF_DROPTAIL_H
#define SCHEDULER_BENCHMARK_SERVER_POLICIES_POLICY_WSJF_DROPTAIL_H

// Library headers
#include "heaps/fibonacci_heap.hpp"

// STD headers
#include <stdint.h>
#include <string>

// DPDK headers
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_ring.h>

/**
 * Implements the WSJF scheduling policy using a
 * Fibonacci heap, dropping packets at the tail.
 */
class PolicyWSJFFibonacciDropTail {
private:
    uint64_t num_rx_ = 0;
    struct rte_ring* process_ring_;
    struct rte_mempool* mbuf_pool_;
    FibonacciHeap<rte_mbuf*> queue_;

public:
    static std::string name();
    PolicyWSJFFibonacciDropTail(struct rte_mempool* mbuf_pool,
                                struct rte_ring* process_ring);
    void scheduleBurst();
    void enqueueBurst(struct rte_mbuf** mbufs,
                      const uint16_t num_mbufs);
};

#endif // SCHEDULER_BENCHMARK_SERVER_POLICIES_POLICY_WSJF_DROPTAIL_H
