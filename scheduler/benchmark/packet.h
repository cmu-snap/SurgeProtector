#ifndef SCHEDULER_BENCHMARK_PACKET_H
#define SCHEDULER_BENCHMARK_PACKET_H

// DPDK headers
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_udp.h>

/**
 * Traffic parameters.
 */
// Packet size
constexpr uint32_t kCommonPSize = (sizeof(struct rte_ether_hdr) +
                                   sizeof(struct rte_ipv4_hdr) +
                                   sizeof(struct rte_udp_hdr));

constexpr uint32_t kInnocentMaxPayloadSize = 1450;
constexpr uint32_t kInnocentAvgPayloadSize = 1208;
constexpr uint32_t kInnocentStdPayloadSize = 100;
constexpr uint32_t kInnocentAvgPSizeInBytes = (
    kCommonPSize + kInnocentAvgPayloadSize);

constexpr uint32_t kInnocentMinPayloadSize = (
    kInnocentAvgPayloadSize - (
        kInnocentMaxPayloadSize -
        kInnocentAvgPayloadSize)
);

constexpr uint32_t kAttackPayloadSize = 22;
constexpr uint32_t kAttackPSizeInBytes = (
    kCommonPSize + kAttackPayloadSize);

// Job size
constexpr uint32_t kInnocentAvgJSizeInNs = 1000;
constexpr uint32_t kInnocentStdJSizeInNs = 100;
constexpr uint32_t kAttackJSizeInNs = 10000;

// Packet parameters
#define PAYLOAD_JSIZE_OFFSET    0
#define PAYLOAD_CLASS_OFFSET    4
enum PacketClass { ATTACK = 0, INNOCENT };

/**
 * Packet parameters.
 */
struct PacketParams {
    uint8_t class_tag; // Packet class
    uint32_t jsize_ns; // Job size (in ns)
    uint32_t psize_bytes; // Packet size (in bytes)
};

/**
 * Returns parameters corresponding to the given packet mbuf.
 */
static inline PacketParams getPacketParams(rte_mbuf* mbuf) {
    char* payload = rte_pktmbuf_mtod_offset(
        mbuf, char*,
        (sizeof(struct rte_ether_hdr) +
         sizeof(struct rte_ipv4_hdr) +
         sizeof(struct rte_udp_hdr)));

    // Fetch the job size and class
    uint32_t jsize_ns = rte_be_to_cpu_32(
        *((uint32_t*) (payload + PAYLOAD_JSIZE_OFFSET)));

    uint8_t class_tag = (
        *((uint8_t*) (payload + PAYLOAD_CLASS_OFFSET)));

    // Return the packet parameters
    return PacketParams{class_tag, jsize_ns, mbuf->pkt_len};
}

#endif // SCHEDULER_BENCHMARK_PACKET_H
