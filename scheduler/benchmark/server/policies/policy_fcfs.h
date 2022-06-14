#ifndef SCHEDULER_BENCHMARK_SERVER_POLICIES_POLICY_FCFS_H
#define SCHEDULER_BENCHMARK_SERVER_POLICIES_POLICY_FCFS_H

// STD headers
#include <stdint.h>
#include <string>

// DPDK headers
#include <rte_mbuf.h>
#include <rte_mempool.h>
#include <rte_ring.h>

/**
 * Implements the FCFS scheduling policy.
 */
class PolicyFCFS {
private:
    uint64_t num_rx_ = 0;
    struct rte_ring* process_ring_;
    struct rte_mempool* mbuf_pool_;

public:
    ~PolicyFCFS();
    PolicyFCFS(struct rte_mempool* mbuf_pool,
               struct rte_ring* process_ring);

    static std::string name();

    void scheduleBurst();
    void enqueueBurst(struct rte_mbuf** mbufs,
                      const uint16_t num_mbufs);
};

#endif // SCHEDULER_BENCHMARK_SERVER_POLICIES_POLICY_FCFS_H
