#ifndef SIMULATOR_SERVER_SERVER_H
#define SIMULATOR_SERVER_SERVER_H

// Library headers
#include "packet/packet.h"
#include "queueing/base_queue.h"
#include "applications/application.h"

/**
 * Represents a single, non-preemptive server.
 */
class Server {
private:
    // Underlying application
    Application* app_ = nullptr;

    // Housekeeping
    bool is_busy_ = false; // Server busy?
    Packet packet_; // Packet currently being served
    double depart_time_ = 0; // Departure time for packet

public:
    explicit Server(Application* a, const BaseQueue* q);
    ~Server();

    // Accessor methods
    bool isBusy() const { return is_busy_; }
    Application* getApplication() const { return app_; }
    double getDepartureTime() const { return depart_time_; }

    /**
     * Sets the estimated & actual job sizes for the parameterized packet.
     * Note: This method MUST be invoked on each packet before scheduling
     * it or inserting it into the packet queue. As a corollary, packets'
     * job size fields must not be used prior to this invocation.
     */
    void setJobSizeEstimateAndActual(Packet& packet);

    /**
     * Record packet departure.
     */
    Packet recordDeparture();

    /**
     * Schedule a new packet.
     */
    void schedule(const double time, Packet packet);
};

#endif // SIMULATOR_SERVER_SERVER_H
