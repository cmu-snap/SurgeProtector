#include "sjf_queue.h"

Packet SJFQueue::pop() {
    BaseQueue::assertNotEmpty(empty());
    Packet packet = queue_.top().tag();
    queue_.pop();
    return packet;
}

Packet SJFQueue::peek() const {
    BaseQueue::assertNotEmpty(empty());
    return queue_.top().tag();
}

void SJFQueue::push(const Packet& packet) {
    SJFPriorityEntry entry(packet, packet.getJobSizeEstimate());
    queue_.push(entry);
}
