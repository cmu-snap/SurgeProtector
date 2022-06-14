#ifndef SIMULATOR_APPLICATIONS_ECHO_H
#define SIMULATOR_APPLICATIONS_ECHO_H

// Library headers
#include "application.h"

/**
 * A simple echo application. Uses the trace-specified job size.
 */
class Echo final : public Application {
public:
    virtual ~Echo() {}
    explicit Echo(const Application::Parameters& p) :
                  Application(name(), p) {}
    /**
     * Returns the application name.
     */
    static std::string name() { return "echo"; };

    /**
     * Returns whether this application requires flow ordering.
     */
    virtual bool isFlowOrderRequired() const override { return false; }

    /**
     * Processes the given network packet by invoking the application-
     * specific implementation and returns the actual job size (in ns).
     */
    virtual double process(const Packet& packet) override;

    /**
     * Returns the estimated time (in ns) to process the packet.
     */
    virtual double getJobSizeEstimate(const Packet& packet) override;
};

#endif // SIMULATOR_APPLICATIONS_ECHO_H
