#include "server.h"

// Library headers
#include "common/utils.h"
#include "packet/packet.h"

// STD headers
#include <assert.h>

Server::Server(Application* a, const BaseQueue* q) : app_(a) {
    // Ensure that the application's requirements
    // of per-flow packet ordering are respected.
    if (app_->isFlowOrderRequired() &&
        !q->isFlowOrderMaintained()) {
        throw std::runtime_error(
            "Policy " + q->type() + " does not guarantee per-flow " +
            "ordering (required by application " + app_->type() + ")");
    }
}

Server::~Server() { delete(app_); }

/**
 * Sets the actual & estimated job sizes for the given packet.
 */
void Server::setJobSizeEstimateAndActual(Packet& packet) {
    packet.setJobSizeEstimate(app_->getJobSizeEstimate(packet));
    packet.setJobSizeActual(app_->process(packet));
}

/**
 * Record packet departure.
 */
Packet Server::recordDeparture() {
    packet_.setDepartTime(depart_time_);
    is_busy_ = false;
    return packet_;
}

/**
 * Schedule a new packet.
 */
void Server::schedule(const double time, Packet packet) {
    // Sanity checks
    assert(packet.getJobSizeEstimate() >= 0);
    assert(!is_busy_ && time >= depart_time_);
    const double jsize = packet.getJobSizeActual();
    assert(jsize != kInvalidJobSize); // Sanity check

    // Update the current server state
    depart_time_ = time + jsize;
    packet_ = packet;
    is_busy_ = true;
}
