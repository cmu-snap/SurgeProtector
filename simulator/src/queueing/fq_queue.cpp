#include "fq_queue.h"

// STD headers
#include <stdexcept>

/**
 * FQFlowMetadata implementation.
 */
FQFlowMetadata& FQFlowMetadata::push(const Packet& packet) {
    // Update the flow's virtual clock. If the flow queue is not empty
    // when this packet arrives, use the existing virtual clock. Else,
    // compute the virtual clock relative to the arrival time.
    virtual_clock_ = (
        packet.getJobSizeEstimate() +
        ((num_packets_ != 0) ? virtual_clock_ :
                               packet.getArriveTime()));
    num_packets_++;
    return *this;
}

bool FQFlowMetadata::pop() {
    if (num_packets_ == 0) {
        throw std::runtime_error("Cannot pop an empty flow queue.");
    }
    num_packets_--;
    return (num_packets_ == 0);
}

/**
 * FQQueue implementation.
 */
Packet FQQueue::pop() {
    BaseQueue::assertNotEmpty(empty());
    Packet packet = queue_.top().tag();
    queue_.pop(); // Pop the global queue

    // Update the flow metadata
    auto iter = data_.find(packet.getFlowId());
    assert(iter != data_.end()); // Sanity check
    if (iter->second.pop()) { data_.erase(iter); }

    size_--; // Decrement the global queue size
    return packet;
}

Packet FQQueue::peek() const {
    BaseQueue::assertNotEmpty(empty());
    return queue_.top().tag();
}

void FQQueue::push(const Packet& packet) {
    // Fetch (and update) the flow metadata
    const FQFlowMetadata data = data_[
        packet.getFlowId()].push(packet);

    FQPriorityEntry entry(packet, data.getVirtualClock(),
                          packet.getArriveTime());

    queue_.push(entry); // Insert packet into the queue
    size_++; // Increment the global queue size
}
