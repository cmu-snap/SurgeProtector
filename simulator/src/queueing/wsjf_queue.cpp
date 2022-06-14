#include "wsjf_queue.h"

Packet WSJFQueue::pop() {
    BaseQueue::assertNotEmpty(empty());
    Packet packet = queue_.top().tag();
    queue_.pop();
    return packet;
}

Packet WSJFQueue::peek() const {
    BaseQueue::assertNotEmpty(empty());
    return queue_.top().tag();
}

void WSJFQueue::push(const Packet& packet) {
    double metric = (packet.getJobSizeEstimate() /
                     static_cast<double>(packet.getPacketSize()));

    WSJFPriorityEntry entry(packet, metric);
    queue_.push(entry);
}
