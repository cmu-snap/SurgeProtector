#include "application.h"

// STD headers
#include <iomanip>
#include <iostream>

/**
 * Application implementation.
 */
void Application::printConfiguration() const {
    std::cout << "{"
              << std::endl << "\ttype = " << kType << ","
              << std::endl << "\tstsf = "
              << std::fixed << std::setprecision(2)
              << kAppParams.getServiceTimeScaleFactor() << ","
              << std::endl << "\tuse_heuristic = "
              << (kAppParams.getUseHeuristic() ? "true" : "false") << ","
              << std::endl << "\tmax_attack_job_size_ns = "
              << kAppParams.getMaxAttackJobSizeNs()
              << std::endl << "}";
}
