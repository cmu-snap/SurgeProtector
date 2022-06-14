#include "application_factory.h"

// Library headers
#include "common/distributions/distribution_factory.h"
#include "echo.h"
#include "iid_job_sizes.h"
#include "tcp_reassembly.h"

// STD headers
#include <stdexcept>

Application* ApplicationFactory::
generate(const libconfig::Setting& app_config) {
    Application* application = nullptr;
    Application::Parameters parameters;
    {
        // Parse common parameters
        bool use_heuristic = false;
        double service_time_scaling = 1;
        double max_attack_jsize_ns = NAN;

        // Use heuristics?
        app_config.lookupValue("heuristic", use_heuristic);

        // Service time scaling factor
        if (!app_config.lookupValue("stsf", service_time_scaling)) {
            throw std::runtime_error("Must specify 'stsf' (Service Time "
                                     "Scale Factor) for any application.");
        }
        // Maximum job size for attack traffic
        if (!app_config.lookupValue("max_attack_job_size_ns", max_attack_jsize_ns)) {
            throw std::runtime_error("Must specify 'max_attack_job_size_ns' "
                                     "(maximum job size (in ns) an attacker "
                                     "may use) for any application.");
        }
        // Update application parameters
        parameters.setUseHeuristic(use_heuristic);
        parameters.setMaxAttackJobSizeNs(max_attack_jsize_ns);
        parameters.setServiceTimeScaling(service_time_scaling);
    }
    std::string type; // Application type
    if (!app_config.lookupValue("type", type)) {
        throw std::runtime_error("No application type specified.");
    }
    // Echo application
    else if (type == Echo::name()) {
        application = new Echo(parameters);
    }
    // IID job sizes application
    else if (type == IIDJobSizes::name()) {
        // Parse the job size configuration
        if (!app_config.exists("job_size_ns_dist")) {
            throw std::runtime_error(
                "Must specify 'job_size_ns_dist' for IIDJobSizes application.");
        }
        else {
            libconfig::Setting& jsize_config = app_config["job_size_ns_dist"];
            auto jsize_dist = DistributionFactory::generate(jsize_config);
            application = new IIDJobSizes(parameters, jsize_dist);
        }
    }
    // TCP Reassembly application
    else if (type == TCPReassembly::name()) {
        application = new TCPReassembly(parameters);
    }
    // Unknown application
    else { throw std::runtime_error(
        "Unknown application type: " + type + "."); }

    return application;
}
