// Library headers
#include "benchmark/packet.h"
#include "common/macros.h"
#include "common/tsc_clock.h"
#include "policies/policy_fcfs.h"
#include "policies/policy_wsjf_dropmax.h"
#include "policies/policy_wsjf_droptail.h"
#include "policies/policy_wsjf_hffs.h"
#include "policies/scheduler.hpp"

// STD headers
#include <iomanip>
#include <iostream>
#include <signal.h>
#include <stdint.h>
#include <getopt.h>

// DPDK headers
#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_udp.h>

static const struct rte_eth_conf port_conf_default = {
};

volatile bool quit;
static void signal_handler(int signum) {
    SUPPRESS_UNUSED_WARNING(signum);
    quit = true;
}

#define CMD_OPT_HELP "help"
#define CMD_OPT_POLICY "policy"
enum {
    /* long options mapped to short options: first long only option value must
    * be >= 256, so that it does not conflict with short options.
    */
    CMD_OPT_HELP_NUM = 256,
    CMD_OPT_POLICY_NUM
};

static void print_usage(const char* program_name) {
    printf("%s [EAL options] --"
        " [--help] |\n"
        " [--policy POLICY]\n\n"

        "  --help: Show this help and exit\n"
        "  --policy POLICY: Scheduling policy to use\n",
        program_name);
}

/* if we ever need short options, add to this string */
static const char short_options[] = "";

static const struct option long_options[] = {
    {CMD_OPT_HELP, no_argument, NULL, CMD_OPT_HELP_NUM},
    {CMD_OPT_POLICY, required_argument, NULL, CMD_OPT_POLICY_NUM},
    {0, 0, 0, 0}
};

/**
 * Worker lcore configuration.
 */
enum WorkerIdx : uint8_t {
    PROCESS = 0, PROFILE };

struct worker_conf {
    struct rte_ring* process_ring;
    struct rte_ring* profile_ring;

    worker_conf(struct rte_ring* process_ring, struct rte_ring* profile_ring) :
                process_ring(process_ring), profile_ring(profile_ring) {}
};

/**
 * Returns (by reference) a string corresponding to the scheduling
 * policy to use.
 */
int get_policy_name(int argc, char** argv, std::string& policy) {
    int opt;
    int long_index;

    while ((opt = getopt_long(argc, argv, short_options, long_options,
           &long_index)) != EOF) {
        switch (opt) {
            case CMD_OPT_HELP_NUM: {
                return 1;
            }
            case CMD_OPT_POLICY_NUM: {
                policy = std::string(optarg);
                break;
            }
            default: {
                return -1;
            }
        }
    }

    return 0;
}

/*
 * Initializes a given port using global settings.
 */
static inline int
port_init(uint16_t port, struct rte_mempool *mbuf_pool) {
    struct rte_eth_conf port_conf = port_conf_default;
    const uint16_t rx_rings = 1, tx_rings = 1;
    uint16_t nb_rxd = DESC_RING_SIZE;
    uint16_t nb_txd = DESC_RING_SIZE;
    int retval;
    uint16_t q;
    struct rte_eth_dev_info dev_info;
    struct rte_eth_txconf txconf;

    if (!rte_eth_dev_is_valid_port(port))
        return -1;

    retval = rte_eth_dev_info_get(port, &dev_info);
    if (retval != 0) {
        printf("Error during getting device (port %u) info: %s\n",
                port, strerror(-retval));
        return retval;
    }

    if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_MBUF_FAST_FREE)
        port_conf.txmode.offloads |=
            DEV_TX_OFFLOAD_MBUF_FAST_FREE;

    /* Configure the Ethernet device. */
    retval = rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf);
    if (retval != 0)
        return retval;

    retval = rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd);
    if (retval != 0)
        return retval;

    /* Allocate and set up 1 RX queue per Ethernet port. */
    for (q = 0; q < rx_rings; q++) {
        retval = rte_eth_rx_queue_setup(port, q, nb_rxd,
                rte_eth_dev_socket_id(port), NULL, mbuf_pool);
        if (retval < 0)
            return retval;
    }

    txconf = dev_info.default_txconf;
    txconf.offloads = port_conf.txmode.offloads;
    /* Allocate and set up 1 TX queue per Ethernet port. */
    for (q = 0; q < tx_rings; q++) {
        retval = rte_eth_tx_queue_setup(port, q, nb_txd,
                rte_eth_dev_socket_id(port), &txconf);
        if (retval < 0)
            return retval;
    }

    /* Start the Ethernet port. */
    retval = rte_eth_dev_start(port);
    if (retval < 0)
        return retval;

    /* Display the port MAC address. */
    struct rte_ether_addr addr;
    retval = rte_eth_macaddr_get(port, &addr);
    if (retval != 0)
        return retval;

    printf("Port %u MAC: %02" PRIx8 " %02" PRIx8 " %02" PRIx8
           " %02" PRIx8 " %02" PRIx8 " %02" PRIx8 "\n",
            port,
            addr.addr_bytes[0], addr.addr_bytes[1],
            addr.addr_bytes[2], addr.addr_bytes[3],
            addr.addr_bytes[4], addr.addr_bytes[5]);

    /* Enable RX in promiscuous mode for the Ethernet device. */
    retval = rte_eth_promiscuous_enable(port);
    if (retval != 0)
        return retval;

    return 0;
}

/**
 * Profiling lcore.
 */
static int
lcore_profile(void* arg) {
    struct rte_mbuf* mbuf = nullptr;
    auto conf = (struct worker_conf*) arg;

    // Throughput measurement
    uint64_t total_psize_all = 0;
    uint64_t total_psize[2]{0, 0};
    uint64_t period_psize[2]{0, 0};
    uint64_t num_processed_all = 0;
    uint64_t num_processed[2]{0, 0};
    uint64_t ticks_per_us = clock_scale();
    uint64_t ticks_per_s = (ticks_per_us * 1000000);

    // Instantaneous throughput
    uint64_t first_tick = 0;
    uint64_t current_tick = 0;
    uint64_t period_start_tick = TscClock::now();
    uint64_t period_end_tick = (period_start_tick + ticks_per_s);

    // Run until the application is killed
    while (likely(!quit)) {
        if (likely(!rte_ring_empty(conf->profile_ring))) {
            if (unlikely(num_processed_all == 0)) {
                first_tick = TscClock::now();
            }
            // Deque a packet from the profile ring
            rte_ring_sc_dequeue(conf->profile_ring, (void**) &mbuf);
            auto params = getPacketParams(mbuf); // Fetch parameters

            // Deallocate the packet
            rte_pktmbuf_free(mbuf);

            // Update statistics
            num_processed_all++;
            num_processed[params.class_tag]++;
            total_psize_all += (params.psize_bytes * 8);
            total_psize[params.class_tag] += (params.psize_bytes * 8);
            period_psize[params.class_tag] += (params.psize_bytes * 8);
        }

        // Display instantaneous goodput every 1s
        if ((current_tick = TscClock::now()) >= period_end_tick) {
            uint64_t elapsed_ticks = current_tick - period_start_tick;
            double elapsed_ns = (elapsed_ticks * 1000) / ticks_per_us;
            double goodput_gbps = ((double) period_psize[
                PacketClass::INNOCENT] / elapsed_ns);

            std::cout << "Instantaneous goodput: "
                      << goodput_gbps << " Gbps" << std::endl;

            // Next period commences
            period_start_tick = current_tick;
            period_psize[PacketClass::ATTACK] = 0;
            period_psize[PacketClass::INNOCENT] = 0;
            period_end_tick = (period_start_tick + ticks_per_s);
        }
    }
    // Compute the goodput
    uint64_t elapsed_ticks = TscClock::now() - first_tick;
    double elapsed_ns = (elapsed_ticks * 1000) / ticks_per_us;
    double goodput_gbps = ((double) total_psize[PacketClass::INNOCENT] / elapsed_ns);

    std::cout << std::endl
              << "------------------------------------" << std::endl
              << "|       WORKER LCORE (SERVER)      |" << std::endl
              << "------------------------------------" << std::endl;

    std::cout << "Total number of packets: "
              << num_processed_all << std::endl;

    std::cout << "Number of innocent packets: "
              << num_processed[PacketClass::INNOCENT]
              << std::endl;

    std::cout << "Number of attack packets: "
              << num_processed[PacketClass::ATTACK]
              << std::endl;

    std::cout << "Total time elapsed: "
              << std::fixed << std::setprecision(2)
              << elapsed_ns / kNanosecsPerSec << " s"
              << std::endl;

    std::cout << "Packet goodput: "
              << std::fixed << std::setprecision(2)
              << goodput_gbps << " Gbps" << std::endl;

    std::cout << std::endl;
    return 0;
}

/*
 * Processing lcore.
 */
static int
lcore_process(void* arg) {
    struct rte_mbuf* mbuf = nullptr;
    auto conf = (struct worker_conf*) arg;

    // Job size emulation
    uint64_t period_end_tick;
    uint64_t period_start_tick = 0;
    uint64_t ticks_per_us = clock_scale();

    // Run until the application is killed
    while (likely(!quit)) {
        if (unlikely(rte_ring_empty(conf->process_ring))) {
            continue;
        }
        else if (unlikely(period_start_tick == 0)) {
            period_start_tick = TscClock::now();
        }
        // Deque a packet from the process ring
        rte_ring_sc_dequeue(conf->process_ring, (void**) &mbuf);
        auto params = getPacketParams(mbuf); // Fetch parameters

        // Compute the end of the period
        period_end_tick = (period_start_tick +
                           (params.jsize_ns * ticks_per_us) / 1000);
        #ifdef DEBUG
        if (num_processed_all % 10000 == 0) {
            std::cout << "Class: " << (uint32_t) params.class_tag << ", "
                      << "PSize: " << params.psize_bytes << "B, "
                      << "JSize: " << params.jsize_ns << "ns"
                      << std::endl;
        }
        #endif
        // Stall for the required amount of time, then handoff to profiling
        while ((period_start_tick = TscClock::now()) < period_end_tick) {}
        if (rte_ring_sp_enqueue(conf->profile_ring, mbuf) != 0) {
            rte_pktmbuf_free(mbuf);
        };
    }
    return 0;
}

/**
 * Scheduler dispatch routine.
 */
template<typename Policy> void run_scheduler(
    struct rte_mempool* pool, struct rte_ring* process_ring) {
    Scheduler<Policy>(pool, process_ring).run(&quit); // Run scheduler
}

int main(int argc, char *argv[]) {
    struct rte_ring *process_ring = NULL;
    struct rte_ring *profile_ring = NULL;
    struct rte_mempool *mbuf_pool = NULL;
    std::string policy;

    quit = false;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Initialize the Environment Abstraction Layer (EAL). */
    int ret = rte_eal_init(argc, argv);
    if (ret < 0) {
        rte_exit(EXIT_FAILURE, "Error with EAL initialization\n");
    }
    argc -= ret;
    argv += ret;

    // Parse the scheduling policy from the CL arguments
    if (get_policy_name(argc, argv, policy) != 0) {
        print_usage(argv[0]);
        if (ret == 1) { return 0; }
        rte_exit(EXIT_FAILURE, "Invalid CLI options\n");
    }
    if (rte_eth_dev_count_avail() != 1) {
        rte_exit(EXIT_FAILURE, "Error: support only for one port\n");
    }
    if (rte_lcore_count() != 3) {
        rte_exit(EXIT_FAILURE, "Error: lcore_count must be 3\n");
    }

    unsigned mbuf_entries = (BURST_SIZE +
                             MBUF_CACHE_SIZE +
                             SCHEDULER_QUEUE_SIZE +
                             (2 * DESC_RING_SIZE) +
                             (2 * PROCESS_RING_SIZE));

    mbuf_entries = RTE_MAX(mbuf_entries, (unsigned) MIN_NUM_MBUFS);
    /* Creates a new mempool in memory to hold the mbufs. */
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", mbuf_entries,
        MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Failed to create mbuf pool\n");

    /* Initialize all ports. */
    if (port_init(0, mbuf_pool))
        rte_exit(EXIT_FAILURE, "Cannot init port %" PRIu16 "\n", 0);

    // Create the process ring
    unsigned lcore_id;
    uint8_t worker_idx = 0;
    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        if (worker_idx == WorkerIdx::PROCESS) {
            // If using an FCFS scheduler, the scheduler queue and
            // process ring are one and the same. In this case, we
            // use the queue length to size the process ringbuffer.
            const bool is_fcfs = (policy == PolicyFCFS::name());
            unsigned int pr_size = is_fcfs ? SCHEDULER_QUEUE_SIZE :
                                             PROCESS_RING_SIZE;

            process_ring = rte_ring_create("process_ring", pr_size,
                                           rte_lcore_to_socket_id(lcore_id),
                                           (RING_F_SP_ENQ | RING_F_SC_DEQ));
            if (process_ring == NULL) {
                rte_exit(EXIT_FAILURE, "Failed to create process ring\n");
            }
        }
        else if (worker_idx == WorkerIdx::PROFILE) {
            // Create the profile ring
            profile_ring = rte_ring_create("profile_ring", BURST_SIZE,
                                           rte_lcore_to_socket_id(lcore_id),
                                           (RING_F_SP_ENQ | RING_F_SC_DEQ));
            if (profile_ring == NULL) {
                rte_exit(EXIT_FAILURE, "Failed to create profile ring\n");
            }
        }
        else { rte_exit(EXIT_FAILURE, "Too many lcores\n"); }
        worker_idx++;
    }

    // Run the worker process
    worker_idx = 0;
    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        if (worker_idx == WorkerIdx::PROCESS) {
            rte_eal_remote_launch(lcore_process, (void*) (
                new worker_conf(process_ring, profile_ring)), lcore_id);
        }
        else if (worker_idx == WorkerIdx::PROFILE) {
            rte_eal_remote_launch(lcore_profile, (void*) (
                new worker_conf(process_ring, profile_ring)), lcore_id);
        }
        else { rte_exit(EXIT_FAILURE, "Too many lcores\n"); }
        worker_idx++;
    }
    // Run the scheduler process
    if (policy == PolicyFCFS::name()) {
        run_scheduler<PolicyFCFS>(mbuf_pool, process_ring);
    }
    else if (policy == PolicyWSJFFibonacciDropMax::name()) {
        run_scheduler<PolicyWSJFFibonacciDropMax>(mbuf_pool, process_ring);
    }
    else if (policy == PolicyWSJFFibonacciDropTail::name()) {
        run_scheduler<PolicyWSJFFibonacciDropTail>(mbuf_pool, process_ring);
    }
    else if (policy == PolicyWSJFFHierarchicalFFS::name()) {
        run_scheduler<PolicyWSJFFHierarchicalFFS>(mbuf_pool, process_ring);
    }
    else {
        quit = true;
        sleep(1); // Sleep to avoid output mangling
        rte_exit(EXIT_FAILURE, "Unimplemented scheduler policy\n");
    }

    // Wait for all processes to complete
    rte_eal_mp_wait_lcore();

    return 0;
}
