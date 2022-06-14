#ifndef SCHEDULER_BENCHMARK_SERVER_POLICIES_SCHEDULER_HPP
#define SCHEDULER_BENCHMARK_SERVER_POLICIES_SCHEDULER_HPP

// STD headers
#include <iostream>
#include <stdint.h>

// DPDK headers
#include <rte_eal.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>
#include <rte_ring.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>

// DPDK-related macros
#define BURST_SIZE              32
#define MBUF_CACHE_SIZE         512
#define MIN_NUM_MBUFS           8192
#define DESC_RING_SIZE          1024
#define SCHEDULER_QUEUE_SIZE    8192
#define PROCESS_RING_SIZE       (BURST_SIZE)

/**
 * Represents a packet scheduler.
 */
template<typename Policy>
class Scheduler {
private:
    Policy policy_;
    uint64_t num_total_rx_ = 0;
    struct rte_mempool* mbuf_pool_;

public:
    Scheduler(struct rte_mempool* pool, struct rte_ring* pr) :
              policy_(pool, pr), mbuf_pool_(pool) {}

    void run(volatile bool *quit) {
        if (rte_eth_dev_socket_id(0) > 0 &&
            rte_eth_dev_socket_id(0) != (int) rte_socket_id()) {
                std::cout << "[Scheduler] WARNING, port 0 is on remote "
                          << "NUMA node to RX thread. Performance will "
                          << "not be optimal." << std::endl;
        }
        std::cout << "[Scheduler] Policy: "
                  << policy_.name() << std::endl;

        struct rte_mbuf *bufs[BURST_SIZE];
        while (likely(!(*quit))) {
            // Schedule a burst of packets
            policy_.scheduleBurst();

            // Fetch a burst of RX packets and push them onto the packet queue
            const uint16_t num_rx = rte_eth_rx_burst(0, 0, bufs, BURST_SIZE);
            if (likely(num_rx != 0)) {
                num_total_rx_ += num_rx;
                policy_.enqueueBurst(bufs, num_rx);
            }
        }
    }
};

#endif // SCHEDULER_BENCHMARK_SERVER_POLICIES_SCHEDULER_HPP
