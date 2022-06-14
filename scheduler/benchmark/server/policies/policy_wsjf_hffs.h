#ifndef SCHEDULER_BENCHMARK_SERVER_POLICIES_POLICY_WSJF_HFFS_H
#define SCHEDULER_BENCHMARK_SERVER_POLICIES_POLICY_WSJF_HFFS_H

// Library headers
#include "heaps/hffs_queue/software/hffs_queue.hpp"

// STD headers
#include <stdint.h>
#include <string>

// DPDK headers
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_ring.h>

/**
 * Implements the WSJF scheduling policy using
 * a Hierarchical Find-First Set (FFS) queue.
 */
class PolicyWSJFFHierarchicalFFS {
private:
    uint64_t num_rx_ = 0;
    struct rte_ring* process_ring_;
    struct rte_mempool* mbuf_pool_;
    HierarchicalFindFirstSetQueue<rte_mbuf*, uint32_t> queue_;

public:
    static std::string name();
    PolicyWSJFFHierarchicalFFS(struct rte_mempool* mbuf_pool,
                               struct rte_ring* process_ring);
    void scheduleBurst();
    void enqueueBurst(struct rte_mbuf** mbufs,
                      const uint16_t num_mbufs);
};

#endif // SCHEDULER_BENCHMARK_SERVER_POLICIES_POLICY_WSJF_HFFS_H
