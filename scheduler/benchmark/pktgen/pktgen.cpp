// Library headers
#include "benchmark/packet.h"
#include "common/distributions/normal_distribution.h"
#include "common/tsc_clock.h"

// STD headers
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <signal.h>
#include <stdint.h>
#include <time.h>
#include <unordered_map>

// DPDK headers
#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_udp.h>

// DPDK-related macros
#define BURST_SIZE              32
#define MBUF_CACHE_SIZE         512
#define MIN_NUM_MBUFS           8192
#define DESC_RING_SIZE          1024

static const struct rte_eth_conf port_conf_default = {
};

volatile bool quit;
static void signal_handler(int signum) {
    SUPPRESS_UNUSED_WARNING(signum);
    quit = true;
}

// Source/destination ports
constexpr uint16_t src_port = 8091;
constexpr uint16_t dst_port = 8091;

// Source/destination IPs
constexpr uint32_t src_ip = 171704321; // 10.60.0.1
constexpr uint32_t dst_ip = 171048961; // 10.50.0.1

// Source/destination MACs
struct rte_ether_addr src_mac = {{0xb4, 0x96, 0x91, 0xa4, 0x02, 0xe9}};
struct rte_ether_addr dst_mac = {{0xb4, 0x96, 0x91, 0xa4, 0x04, 0x21}};

// Typedefs
typedef std::unordered_map<uint16_t, uint16_t> PayloadSizeToIPChecksumMap;

#define CMD_OPT_HELP "help"
#define CMD_OPT_RATE_ATTACK "rate-attack"
#define CMD_OPT_RATE_INNOCENT "rate-innocent"
enum {
    /* long options mapped to short options: first long only option value must
    * be >= 256, so that it does not conflict with short options.
    */
    CMD_OPT_HELP_NUM = 256,
    CMD_OPT_RATE_ATTACK_NUM,
    CMD_OPT_RATE_INNOCENT_NUM
};

static void print_usage(const char* program_name) {
    printf("%s [EAL options] --"
        " [--help] |\n"
        " [--rate-attack RATE_ATTACK]\n"
        " [--rate-innocent RATE_INNOCENT]\n\n"

        "  --help: Show this help and exit\n"
        "  --rate-attack RATE_ATTACK: Rate (in Gbps) of attack traffic\n"
        "  --rate-innocent RATE_INNOCENT: Rate (in Gbps) of innocent traffic\n",
        program_name);
}

/* if we ever need short options, add to this string */
static const char short_options[] = "";

static const struct option long_options[] = {
    {CMD_OPT_HELP, no_argument, NULL, CMD_OPT_HELP_NUM},
    {CMD_OPT_RATE_ATTACK, required_argument, NULL, CMD_OPT_RATE_ATTACK_NUM},
    {CMD_OPT_RATE_INNOCENT, required_argument, NULL, CMD_OPT_RATE_INNOCENT_NUM},
    {0, 0, 0, 0}
};

// Command-line arguments
struct cl_arguments {
    double attack_rate_gbps;
    double innocent_rate_gbps;
};

// Worker configuration
struct worker_conf {
    rte_mempool* pool;
    uint16_t class_tag;
    double tx_rate_gbps;
    uint32_t avg_psize_bytes;

    worker_conf(rte_mempool* pool, uint16_t class_tag, double rate_gbps,
                uint32_t avg_psize_bytes) : pool(pool), class_tag(class_tag),
                tx_rate_gbps(rate_gbps), avg_psize_bytes(avg_psize_bytes) {}
};

/**
 * Given a desired throughput and (expected) packet size,
 * computes the number of TSC ticks per packet burst.
 */
static inline uint64_t
compute_ticks_per_burst(double rate_gbps, uint32_t psize_bits) {
    if (rate_gbps == 0) { return 0; } // Traffic-gen is disabled
    double packets_per_us = (rate_gbps * 1000 / psize_bits);
    uint64_t ticks_per_us = clock_scale();

    // (ticks/us * packets/burst) / (packets/us)
    return (ticks_per_us * BURST_SIZE) / packets_per_us;
}

/**
 * Generates a template packet.
 */
static inline struct rte_mbuf*
generate_template_packet(struct rte_mempool* pool) {
    // Allocate a new mbuf corresponding to the retval
    struct rte_mbuf* mbuf = rte_pktmbuf_alloc(pool);
    if (unlikely(mbuf == NULL)) { return NULL; }

    // Initialize Ethernet header
    struct rte_ether_hdr* ether_hdr =
        rte_pktmbuf_mtod(mbuf, struct rte_ether_hdr*);

    ether_hdr->s_addr = src_mac;
    ether_hdr->d_addr = dst_mac;
    ether_hdr->ether_type = rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4);

    // Initialize the IPv4 header
    struct rte_ipv4_hdr* ip_hdr = (struct rte_ipv4_hdr*) (
        ((char*) ether_hdr) + sizeof(struct rte_ether_hdr));

    ip_hdr->version_ihl = RTE_IPV4_VHL_DEF;
    ip_hdr->type_of_service = 0;
    ip_hdr->total_length = 0; // Parameter
    ip_hdr->packet_id = 0;
    ip_hdr->fragment_offset = 0;
    ip_hdr->time_to_live   = 64;
    ip_hdr->next_proto_id = IPPROTO_UDP;
    ip_hdr->hdr_checksum = 0; // Parameter
    ip_hdr->src_addr = rte_cpu_to_be_32(src_ip);
    ip_hdr->dst_addr = rte_cpu_to_be_32(dst_ip);

    // Initialize the UDP header
    struct rte_udp_hdr* udp_hdr = (struct rte_udp_hdr*) (
        ((char*) ip_hdr) + sizeof(struct rte_ipv4_hdr));

    udp_hdr->src_port = rte_cpu_to_be_16(src_port);
    udp_hdr->dst_port = rte_cpu_to_be_16(dst_port);
    udp_hdr->dgram_len = 0; // Parameter
    udp_hdr->dgram_cksum = 0; // Unused

    return mbuf;
}

/**
 * Allocates and returns a clone of the template packet.
 */
static inline struct rte_mbuf*
clone_template_packet(struct rte_mempool* pool,
                      struct rte_mbuf* template_packet) {
    // Allocate a new mbuf corresponding to the retval
    struct rte_mbuf* mbuf = rte_pktmbuf_alloc(pool);
    if (unlikely(mbuf == NULL)) { return NULL; }

    char* src = ((char*) template_packet->buf_addr) + template_packet->data_off;
    char* dst = ((char*) mbuf->buf_addr) + mbuf->data_off;
    memcpy(dst, src, kCommonPSize);
    return mbuf;
}

/**
 * Updates packet headers based on the given payload size.
 */
static inline void
update_headers(struct rte_mbuf* mbuf,
               uint16_t payload_size,
               PayloadSizeToIPChecksumMap& csum_map) {

    // Fetch the IPv4 and UDP headers
    struct rte_ipv4_hdr* ip_hdr = rte_pktmbuf_mtod_offset(
        mbuf, struct rte_ipv4_hdr*, sizeof(struct rte_ether_hdr));

    struct rte_udp_hdr* udp_hdr = (struct rte_udp_hdr*) (
        ((char*) ip_hdr) + sizeof(struct rte_ipv4_hdr));

    // Compute and update the UDP length
    uint16_t udp_len = payload_size + sizeof(struct rte_udp_hdr);
    udp_hdr->dgram_len = udp_len;

    // Compute and update the IPv4 length and checksum
    uint16_t ip_len = udp_len + sizeof(struct rte_ipv4_hdr);
    ip_hdr->total_length = ip_len;
    uint16_t ip_checksum = 0;

    // If required, compute and memoize the checksum
    auto iter = csum_map.find(ip_len);
    if (iter == csum_map.end()) {
        ip_checksum = rte_ipv4_cksum(ip_hdr);
        csum_map[ip_len] = ip_checksum;
    }
    // Else, use the precomputed checksum
    else { ip_checksum = iter->second; }
    ip_hdr->hdr_checksum = ip_checksum;
}

/**
 * Generates an adversarial packet.
 */
static inline struct rte_mbuf*
generate_attack_packet(struct rte_mempool* pool,
                       struct rte_mbuf* template_packet,
                       PayloadSizeToIPChecksumMap& csum_map) {
    // Allocate a clone of the template packet, then update headers
    rte_mbuf* mbuf = clone_template_packet(pool, template_packet);
    if (unlikely(mbuf == NULL)) { return NULL; }
    update_headers(mbuf, kAttackPayloadSize, csum_map);

    // Write the payload
    char* payload = rte_pktmbuf_mtod_offset(mbuf, char*, kCommonPSize);
    *((uint32_t*) (payload + PAYLOAD_JSIZE_OFFSET)) = (
        rte_cpu_to_be_32(kAttackJSizeInNs));

    *((uint8_t*) (payload + PAYLOAD_CLASS_OFFSET)) = PacketClass::ATTACK;

    // Set the mbuf parameters
    mbuf->pkt_len = kAttackPSizeInBytes;
    mbuf->data_len = kAttackPSizeInBytes;

    return mbuf;
}

/**
 * Populates an innocent packet.
 */
static inline struct rte_mbuf*
generate_innocent_packet(struct rte_mempool* pool,
                         const uint32_t job_size_ns,
                         const uint32_t payload_size,
                         struct rte_mbuf* template_packet,
                         PayloadSizeToIPChecksumMap& csum_map) {
    // Allocate a clone of the template packet, then update headers
    rte_mbuf* mbuf = clone_template_packet(pool, template_packet);
    if (unlikely(mbuf == NULL)) { return NULL; }
    update_headers(mbuf, payload_size, csum_map);

    // Write the payload
    char* payload = rte_pktmbuf_mtod_offset(mbuf, char*, kCommonPSize);
    *((uint32_t*) (payload + PAYLOAD_JSIZE_OFFSET)) = (
        rte_cpu_to_be_32(job_size_ns));

    *((uint8_t*) (payload + PAYLOAD_CLASS_OFFSET)) = PacketClass::INNOCENT;

    // Set the packet parameters
    const uint32_t psize = (kCommonPSize + payload_size);
    mbuf->data_len = psize;
    mbuf->pkt_len = psize;

    return mbuf;
}

/**
 * Returns the parsed command-line arguments.
 */
int get_cl_arguments(int argc, char** argv,
                     cl_arguments& cl_args) {
    int opt;
    int long_index;

    cl_args.attack_rate_gbps = 0;
    cl_args.innocent_rate_gbps = 0;

    while ((opt = getopt_long(argc, argv, short_options,
                    long_options, &long_index)) != EOF) {
        switch (opt) {
            case CMD_OPT_HELP_NUM: {
                return 1;
            }
            case CMD_OPT_RATE_ATTACK_NUM: {
                cl_args.attack_rate_gbps = atof(optarg);
                break;
            }
            case CMD_OPT_RATE_INNOCENT_NUM: {
                cl_args.innocent_rate_gbps = atof(optarg);
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
    const uint16_t rx_rings = 1, tx_rings = 2;
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
    /* Allocate and set up 2 TX queue per Ethernet port. */
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

    return 0;
}

/*
 * Worker lcore.
 */
static int
lcore_worker(void* arg) {
    auto conf = (struct worker_conf*) arg;
    rte_mbuf* mbufs[BURST_SIZE]; // Packet buffers

    // If disabled, do nothing
    if (conf->tx_rate_gbps == 0) { return 0; }

    // Rate-limiting
    uint64_t ticks_per_burst = compute_ticks_per_burst(
        conf->tx_rate_gbps, (conf->avg_psize_bytes * 8));

    // Throughput measurement
    uint64_t num_total_tx = 0;
    uint64_t total_psize_bytes = 0;
    const bool is_attack = (conf->class_tag == PacketClass::ATTACK);
    const std::string class_tag = is_attack ? "Attack" : "Innocent";

    // Packet generation
    NormalDistribution psize_dist(kInnocentAvgPayloadSize,
                                  kInnocentStdPayloadSize,
                                  kInnocentMinPayloadSize,
                                  kInnocentMaxPayloadSize);

    NormalDistribution jsize_dist(kInnocentAvgJSizeInNs,
                                  kInnocentStdJSizeInNs, 0,
                                  (2 * kInnocentAvgJSizeInNs));

    PayloadSizeToIPChecksumMap ip_csum_map; // Payload size -> IPv4 checksum
    struct rte_mbuf* template_packet = generate_template_packet(conf->pool);

    // Rate control
    uint64_t period_end_tick;
    uint64_t first_tick = TscClock::now();
    uint64_t period_start_tick = first_tick;

    // Run until the application is killed
    while (likely(!quit)) {
        period_end_tick = (period_start_tick + ticks_per_burst);

        // Generate a burst of packets
        for (unsigned i = 0; i < BURST_SIZE; i++) {
            mbufs[i] = is_attack ?
                generate_attack_packet(conf->pool, template_packet, ip_csum_map) :
                generate_innocent_packet(conf->pool,
                                         (uint32_t) jsize_dist.sample(),
                                         (uint32_t) psize_dist.sample(),
                                         template_packet, ip_csum_map);

            total_psize_bytes += mbufs[i]->pkt_len;
        }
        uint16_t num_tx = rte_eth_tx_burst(
            0, conf->class_tag, mbufs, BURST_SIZE);

        num_total_tx += num_tx;
        for (auto idx = num_tx; idx < BURST_SIZE; idx++) {
            total_psize_bytes -= mbufs[idx]->pkt_len;
            rte_pktmbuf_free(mbufs[idx]);
        }
        while ((period_start_tick = TscClock::now()) < period_end_tick) {}
    }
    // Compute the throughput
    uint64_t total_psize = (total_psize_bytes * 8);
    uint64_t elapsed_ticks = TscClock::now() - first_tick;
    double elapsed_ns = (elapsed_ticks * 1000) / clock_scale();
    double throughput_gbps = ((double) total_psize) / elapsed_ns;

    struct timespec tspec{conf->class_tag, 0};
    nanosleep(&tspec, NULL); // Random sleep to avoid output mangling

    std::cout << std::endl
              << "------------------------------------" << std::endl
              << "|       WORKER LCORE (PKTGEN)      |" << std::endl
              << "------------------------------------" << std::endl;

    std::cout << "Packet type: " << class_tag << std::endl;
    std::cout << "Ticks per burst: " << ticks_per_burst << std::endl;
    std::cout << "Number of TX packets: " << num_total_tx << std::endl;

    std::cout << "Total time elapsed: "
              << std::fixed << std::setprecision(2)
              << elapsed_ns / kNanosecsPerSec << " s"
              << std::endl;

    std::cout << "Packet throughput: "
              << std::fixed << std::setprecision(2)
              << throughput_gbps << " Gbps" << std::endl;

    std::cout << std::endl;
    return 0;
}

int main(int argc, char *argv[]) {
    struct rte_mempool *mbuf_pool;

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

    // Parse command-line arguments
    cl_arguments cl_args;
    if (get_cl_arguments(argc, argv, cl_args) != 0) {
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

    unsigned mbuf_entries = (MBUF_CACHE_SIZE +
                             (3 * BURST_SIZE) +
                             (3 * DESC_RING_SIZE));

    mbuf_entries = RTE_MAX(mbuf_entries, (unsigned) MIN_NUM_MBUFS);
    /* Creates a new mempool in memory to hold the mbufs. */
    mbuf_pool = rte_pktmbuf_pool_create("MBUF_POOL", mbuf_entries,
        MBUF_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());

    if (mbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Failed to create mbuf pool\n");

    /* Initialize all ports. */
    if (port_init(0, mbuf_pool))
        rte_exit(EXIT_FAILURE, "Cannot init port %" PRIu16 "\n", 0);

    // Create the configurations
    worker_conf* confs[2];
    confs[PacketClass::ATTACK] = new worker_conf(mbuf_pool,
        PacketClass::ATTACK, cl_args.attack_rate_gbps,
        kAttackPSizeInBytes);

    confs[PacketClass::INNOCENT] = new worker_conf(mbuf_pool,
        PacketClass::INNOCENT, cl_args.innocent_rate_gbps,
        kInnocentAvgPSizeInBytes);

    // Run the worker process
    unsigned idx = 0;
    unsigned lcore_id;
    RTE_LCORE_FOREACH_WORKER(lcore_id) {
        rte_eal_remote_launch(lcore_worker,
            (void*) confs[idx], lcore_id);
        idx++;
    }

    // Wait for all processes to complete
    rte_eal_mp_wait_lcore();

    return 0;
}
