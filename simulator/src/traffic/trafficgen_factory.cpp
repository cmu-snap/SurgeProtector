#include "trafficgen_factory.h"

// Library headers
#include "common/distributions/distribution_factory.h"
#include "common/macros.h"
#include "synthetic_trafficgen.h"
#include "trace_trafficgen.h"

// STD headers
#include <iostream>
#include <string.h>

TrafficGenerator* TrafficGeneratorFactory::generate(
    const bool is_dry_run, const TrafficClass tg_type,
    const libconfig::Setting& tg_config,
    const uint32_t fid_start_offset) {
    TrafficGenerator* tg = nullptr;

    // Default inter-arrival time for dry-runs
    const double kDryRunIATInNs = 1000;

    std::string type; // Trafficgen type
    if (!tg_config.lookupValue("type", type)) {
        throw std::runtime_error("No traffic-gen type specified.");
    }
    // Trace-driven traffic-generator
    else if (type == TraceTrafficGenerator::name()) {
        if (tg_type == TrafficClass::ATTACK) {
            throw std::runtime_error("Adversarial traffic-generators must "
                                     "be synthetic (not trace-driven).");
        }
        std::string trace_fp; // Trace file-path
        if (!tg_config.lookupValue("trace_fp", trace_fp)) {
            throw std::runtime_error("Must specify 'trace_fp' for "
                                     "trace-driven traffic-gens.");
        }
        double avg_psize = NAN, rate = NAN; // Average psize (b), rate (bps)
        tg_config.lookupValue("average_packet_size_bits", avg_psize);
        tg_config.lookupValue("rate_bps", rate);

        // Compute the average IAT
        bool is_calibrated = false;
        double iat_ns = kDryRunIATInNs;
        if (!std::isnan(avg_psize) && !std::isnan(rate) && !is_dry_run) {
            // If both the average packet size and rate parameters are
            // specified, then the traffic-generator can be calibrated.
            // Else, the simulator will run in "dry-run" mode.
            iat_ns = (kNanosecsPerSec * avg_psize) / rate;
            is_calibrated = true;
        }

        ConstantDistribution* iat_dist = new ConstantDistribution(iat_ns);
        auto trace_tg = new TraceTrafficGenerator(trace_fp, iat_dist);
        if (is_calibrated) { trace_tg->calibrate(avg_psize); }
        tg = trace_tg;
    }
    // Synthetic traffic-generator
    else if (type == SyntheticTrafficGenerator::name()) {
        // Parse the flow count
        uint32_t nflows = 1;
        tg_config.lookupValue("num_flows", nflows);

        // Innocent traffic
        if (tg_type == TrafficClass::INNOCENT) {
            if (!tg_config.exists("packet_size_bits_dist")) {
            throw std::runtime_error("Must specify 'packet_size_bits_dist' "
                                     "for synthetic traffic-generators.");
            }
            // Parse packet-size distribution
            libconfig::Setting& psize_config = tg_config["packet_size_bits_dist"];
            auto psize_dist = DistributionFactory::generate(psize_config);
            const auto avg_psize = psize_dist->getSampleStats().getMean();

            // Parse the rate (bps)
            double rate = NAN;
            tg_config.lookupValue("rate_bps", rate);

            // Compute the average IAT
            bool is_calibrated = false;
            double iat_ns = kDryRunIATInNs;
            if (!std::isnan(rate) && !is_dry_run) {
                is_calibrated = true;
                iat_ns = (kNanosecsPerSec * avg_psize) / rate;
            }

            ConstantDistribution* iat_dist = new ConstantDistribution(iat_ns);
            auto synthetic_tg = new InnocentTrafficGenerator(
                nflows, iat_dist, psize_dist);

            if (is_calibrated) {synthetic_tg->calibrate(rate); }
            tg = synthetic_tg;
        }
        // Attack traffic
        else {
            // Parse the rate (bps)
            double rate = 0; // Default (no traffic)
            tg_config.lookupValue("rate_bps", rate);

            // Compute the average IAT
            double iat_ns = kDblPosInfty;
            uint32_t attack_psize_bits = 0;
            double attack_jsize_ns = kInvalidJobSize;

            if (rate > 0 && !is_dry_run) {
                if (!tg_config.lookupValue("job_size_ns", attack_jsize_ns)) {
                    throw std::runtime_error("Must specify 'job_size_ns' for "
                                             "attack traffic-generators when "
                                             "not running in dry-run mode.");
                }
                if (!tg_config.lookupValue("packet_size_bits", attack_psize_bits)) {
                    throw std::runtime_error("Must specify 'packet_size_bits' for "
                                             "attack traffic-generators when not "
                                             "running in dry-run mode.");
                }
                iat_ns = (kNanosecsPerSec * attack_psize_bits) / rate;
            }
            else if (rate > 0) {
                std::cout << "Warning: In dry-run mode, but adversarial rate is "
                          << "non-zero. No attack traffic will be generated."
                          << std::endl;
            }

            ConstantDistribution* iat_dist = new ConstantDistribution(iat_ns);
            auto attack_tg = new AttackTrafficGenerator(nflows, fid_start_offset, iat_dist,
                                                        attack_psize_bits, attack_jsize_ns);
            attack_tg->calibrate(rate);
            tg = attack_tg;
        }
    }
    // Unknown traffic-generator
    else { throw std::runtime_error(
        "Unknown traffic-gen type: " + type + "."); }

    // Perform validation. The traffic-generator
    // may be uncalibrated ONLY in dry-run mode.
    if (!is_dry_run && !tg->isCalibrated()) {
        throw std::runtime_error("Traffic-generator must be calibrated (have "
                                 "a valid rate and average packet size) when "
                                 "not running in dry-run mode.");
    }
    return tg;
}
