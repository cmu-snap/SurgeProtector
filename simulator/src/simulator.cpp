#include "simulator.h"

// Library headers
#include "applications/application_factory.h"
#include "common/utils.h"
#include "packet/packet.h"
#include "queueing/fcfs_queue.h"
#include "queueing/queue_factory.h"
#include "server/server.h"
#include "traffic/synthetic_trafficgen.h"
#include "traffic/trace_trafficgen.h"
#include "traffic/trafficgen_factory.h"

// STD headers
#include <exception>
#include <iomanip>
#include <iostream>

// Boost headers
#include <boost/program_options.hpp>

/**
 * Simulator implementation.
 */
Simulator::Simulator(const bool is_dry_run, const
    libconfig::Setting& config) : kIsDryRun(is_dry_run) {
    parseSimulationConfig(config); // Parse the simulation config
}

void Simulator::printConfig() const {
    if (kIsDryRun) { std::cout << "Starting dry run..." << std::endl; }
    std::cout << "==========================================" << std::endl
              << "               Input Config               " << std::endl
              << "==========================================" << std::endl;

    std::cout << "Policy: " << queue_->type() << ","
              << std::endl << "Application: ";
    server_->getApplication()->printConfiguration();

    std::cout << std::endl << "Innocent traffic-gen: ";
    tg_innocent_->printConfiguration();

    if (!kIsDryRun) {
        std::cout << std::endl << "Attack traffic-gen: ";
        tg_attack_->printConfiguration();
    }
    std::cout << std::endl;
}

void Simulator::
parseSimulationConfig(const libconfig::Setting& config) {
    // Parse the maximum arrival count
    kMaxNumArrivals = std::numeric_limits<uint64_t>::max();
    if (config.exists("max_num_arrivals")) {
        kMaxNumArrivals = static_cast<unsigned
            long long>(config.lookup("max_num_arrivals"));
    }

    // Generate the queue
    if (kIsDryRun) {
        std::string policy;
        if (config.lookupValue("policy", policy) && policy != FCFSQueue::name()) {
            std::cout << "'policy' is specified in dry-run mode. "
                      << "Ignoring this and using FCFS instead."
                      << std::endl << std::endl;
        }
        queue_ = new FCFSQueue();
    }
    else {
        queue_ = QueueFactory::generate(config);
    }

    // Generate the application and server
    Application* application = nullptr;
    if (!config.exists("application")) {
        throw std::runtime_error("Must specify 'application'.");
    }
    else {
        application = ApplicationFactory::generate(config["application"]);
    }
    server_ = new Server(application, queue_);

    // Generate the innocent traffic-gen
    if (!config.exists("innocent_traffic")) {
        throw std::runtime_error("Must specify 'innocent_traffic'.");
    }
    else {
        tg_innocent_ = TrafficGeneratorFactory::generate(kIsDryRun,
            TrafficClass::INNOCENT, config["innocent_traffic"]);
    }
    // Generate the attack traffic-gen
    if (!config.exists("attack_traffic")) {
        AttackTrafficGenerator* tg_attack = new AttackTrafficGenerator(
            0, 0, new ConstantDistribution(kDblPosInfty), 0, kInvalidJobSize);

        tg_attack->calibrate(0);
        tg_attack_ = tg_attack;
    }
    else {
        tg_attack_ = TrafficGeneratorFactory::generate(kIsDryRun,
            TrafficClass::ATTACK, config["attack_traffic"],
            tg_innocent_->getNumFlows());
    }
    // Maximum arrival count should be set iff not using a trace
    bool is_use_trace = (tg_innocent_->type() ==
                         TraceTrafficGenerator::name());

    bool is_max_arrival_count_set = (kMaxNumArrivals !=
                                     std::numeric_limits<uint64_t>::max());

    if (!(is_max_arrival_count_set ^ is_use_trace)) {
        throw std::runtime_error(
            "'max_num_arrivals' must be set iff not using a trace.");
    }
}

Simulator::~Simulator() {
    // Deallocate resources
    delete(tg_innocent_);
    delete(tg_attack_);
    delete(server_);
    delete(queue_);
}

void Simulator::run(const bool verbose, const std::string packets_fp) {
    assert(!done); // TODO(natre): Make simulator instances reusable
    std::ofstream packets_of(packets_fp, std::ios::trunc);
    std::vector<Packet> packets; // Packet output

    // Print configuration
    printConfig();

    // Housekeeping
    uint64_t num_arrivals = 0; // Total number of arrivals
    uint64_t num_departures = 0; // Total number of departures
    uint64_t num_innocent_arrivals = 0; // Number of innocent arrivals

    // Profiling
    uint64_t total_psize_i = 0;     // Cumulative packet size (class I traffic)
    double total_jsize_i = 0;       // Cumulative job size (class I traffic)
    uint32_t maximum_psize_i = 0;   // Maximum packet size (class I traffic)
    double maximum_jsize_i = 0;     // Maximum job size (class I traffic)
    double last_arrive_time_i = 0;  // End arrival time (class I traffic)
    double last_depart_time_i = 0;  // End depart time (class I traffic)
    uint64_t ss_total_psize_i = 0;  // Cumulative packet size of class I
                                    // traffic served in steady-state.
    double steady_state_ns = 0;     // Steady-state simulation period

    // More innocent arrivals possible?
    bool more_arrivals = (
        tg_innocent_->hasNewArrival() &&
        (num_innocent_arrivals < kMaxNumArrivals));

    while (more_arrivals || (num_arrivals != num_departures)) {
        // Fetch the arrival and departure times
        bool is_steady_state = more_arrivals;
        double at_attack = tg_attack_->getNextArrivalTime();
        double at_innocent = tg_innocent_->getNextArrivalTime();
        double next_departure_time = server_->getDepartureTime();
        double next_arrival_time = std::min(at_attack, at_innocent);

        // Simulate an arrival
        if (more_arrivals && ((next_arrival_time < next_departure_time) ||
                              !server_->isBusy())) {
            // Next arrival
            TrafficGenerator* tg = (
                (at_attack < at_innocent) ? tg_attack_ : tg_innocent_);

            // Fetch the arrival and compute its estimated job size
            Packet arrival = tg->getNextArrival(num_arrivals);
            server_->setJobSizeEstimateAndActual(arrival);

            // Process jobs with valid job sizes
            if (arrival.getJobSizeActual() != kInvalidJobSize) {
                tg->updateArrivalTime();

                // If the estimated jsize is invalid, reset it to zero
                if (arrival.getJobSizeEstimate() == kInvalidJobSize) {
                    arrival.setJobSizeEstimate(0);
                }

                // If the server is currently unoccupied,
                // schedule the next arrival immediately.
                if (!server_->isBusy()) {
                    assert(queue_->empty());
                    server_->schedule(next_arrival_time, arrival);
                }
                // Else, insert it into the queue
                else { queue_->push(arrival); }

                // Update the steady-state timestamp
                steady_state_ns = next_arrival_time;

                // Increment the innocent and total arrival counts
                bool is_innocent = (arrival.getClass() ==
                                    TrafficClass::INNOCENT);

                if (is_innocent) { num_innocent_arrivals++; }
                num_arrivals++;
            }
        }
        // Simulate a departure
        else {
            assert(server_->isBusy());
            Packet departure = server_->recordDeparture();
            bool is_innocent = (departure.getClass() == TrafficClass::INNOCENT);

            // Update profiling data
            if (is_innocent) {
                total_jsize_i += departure.getJobSizeActual();
                total_psize_i += departure.getPacketSize();
                maximum_jsize_i = std::max(maximum_jsize_i,
                                           departure.getJobSizeActual());

                maximum_psize_i = std::max(maximum_psize_i,
                                           departure.getPacketSize());

                last_depart_time_i = departure.getDepartTime();
                last_arrive_time_i = std::max(last_arrive_time_i,
                                              departure.getArriveTime());
                // Currently in steady-state
                if (is_steady_state) {
                    ss_total_psize_i = total_psize_i;
                }
                // Also log this departure if required
                if (packets_of) { packets.push_back(departure); }
            }

            // If the queue isn't empty, schedule the next packet
            if (!queue_->empty()) {
                server_->schedule(next_departure_time, queue_->pop());
            }
            // Increment the departure count
            num_departures++;
        }

        // More innocent arrivals possible?
        more_arrivals = (
            tg_innocent_->hasNewArrival() &&
            (num_innocent_arrivals < kMaxNumArrivals));
    }
    // Sanity checks
    assert(queue_->empty());
    assert(!server_->isBusy());

    // Compute performance metrics
    uint64_t average_psize_i = (total_psize_i / num_innocent_arrivals);
    double average_jsize_i = (total_jsize_i / num_innocent_arrivals);
    double service_rate_gbps_i = (average_psize_i / average_jsize_i);
    double input_rate_gbps_i = (total_psize_i / last_arrive_time_i);
    double input_rate_gbps_a = (
        tg_attack_->getCalibratedRateInBitsPerSecond() / kBitsPerGb);

    double last_goodput_gbps = (total_psize_i / last_depart_time_i);
    double ss_goodput_gbps = (ss_total_psize_i / steady_state_ns);
    double ss_displacement_factor = 0;

    if (input_rate_gbps_a != 0) {
        // Epsilon to avoid messing with log-scale plots
        ss_displacement_factor = 1e-4;

        // Thresholding to make DF less noisy for small attack input rates
        const double goodput_loss = (input_rate_gbps_i - ss_goodput_gbps);
        if ((goodput_loss / input_rate_gbps_i) > 1e-2) {
            ss_displacement_factor = goodput_loss / input_rate_gbps_a;
        }
    }
    if (verbose) {
        std::cout << "==========================================" << std::endl
                  << "            Simulation Results            " << std::endl
                  << "==========================================" << std::endl;

        // Display performance metrics
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "Maximum service rate: " << service_rate_gbps_i << " Gbps" << std::endl;
        std::cout << "Innocent packet rate: " << input_rate_gbps_i << " Gbps" << std::endl;
        std::cout << "Average packet size: " << average_psize_i << " bits" << std::endl;
        std::cout << "Maximum packet size: " << maximum_psize_i << " bits" << std::endl;
        std::cout << "Average job size: " << average_jsize_i << " ns" << std::endl;
        std::cout << "Maximum job size: " << maximum_jsize_i << " ns" << std::endl;
        std::cout << "Innocent arrivals: " << num_innocent_arrivals << std::endl;
        std::cout << "Total arrivals: " << num_arrivals << std::endl;
        std::cout << std::endl;

        std::cout << std::fixed << std::setprecision(4);
        std::cout << "Average goodput: " << last_goodput_gbps << " Gbps" << std::endl;
        std::cout << "Steady-state goodput: " << ss_goodput_gbps << " Gbps" << std::endl;
        std::cout << "Steady-state displacement factor: " << ss_displacement_factor << std::endl;
        std::cout << std::endl;
    }

    // Output packet data. Note: The output packet
    // log is ordered by packet departure times.
    if (packets_of) {
        packets_of << std::fixed << std::setprecision(2);
        for (const Packet& packet : packets) {
            packets_of << packet.getArriveTime()        << ";"
                       << packet.getDepartTime()        << ";"
                       << packet.getFlowId()            << ";"
                       << packet.getClassTag()          << ";"
                       << packet.getPacketSize()        << ";"
                       << packet.getJobSizeEstimate()   << ";"
                       << packet.getJobSizeActual()     << std::endl;
        }
    }
    // Disallow re-runs
    done = true;
}

int main(int argc, char** argv) {
    using namespace boost::program_options;

    // Command-line arguments
    bool is_dry_run;        // Dry-run mode?
    std::string config_fp;  // Path to config file
    std::string packets_fp; // Path to packets file

    // Parse arguments
    options_description desc{"Adversarial scheduling simulator"};
    variables_map variables;
    try {
        // Command-line arguments
        desc.add_options()
            ("help",    "Prints this message")
            ("config",  value<std::string>(&config_fp)->required(), "[Required] Path to a configuration (.cfg) file")
            ("packets", value<std::string>(&packets_fp),            "[Optional] Path to an output packets file")
            ("dry",                                                 "[Optional] Perform a dry-run (using FCFS)");

        // Parse simulation parameters
        store(command_line_parser(argc, argv).options(desc).run(), variables);

        // Handle help flag
        if (variables.count("help")) {
            std::cout << desc << std::endl;
            return false;
        }
        store(command_line_parser(argc, argv).options(desc).run(), variables);
        notify(variables);
    }
    // Flag argument errors
    catch(const required_option& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return false;
    }
    catch(const error& e) {
        std::cerr << "Error:" << e.what() << std::endl;
        return false;
    }
    // Dry-run?
    is_dry_run = (variables.count("dry") != 0);

    // Parse the configuration
    libconfig::Config cfg;
    try {
        cfg.readFile(config_fp);
    }
    catch(const libconfig::FileIOException &fioex) {
        std::cerr << "I/O error while reading file." << std::endl;
        return(EXIT_FAILURE);
    }
    catch(const libconfig::ParseException &pex) {
        std::cerr << "Parse error at " << pex.getFile() << ":" << pex.getLine()
                  << " - " << pex.getError() << std::endl;
        return(EXIT_FAILURE);
    }

    cfg.setAutoConvert(true);
    Simulator simulator(is_dry_run, cfg.getRoot());
    simulator.run(true, packets_fp); // Run simulation

    return 0;
}
