#include "fcfs_queue.h"

Packet FCFSQueue::pop() {
    BaseQueue::assertNotEmpty(empty());
    Packet packet = queue_.front();
    queue_.pop_front();
    return packet;
}

Packet FCFSQueue::peek() const {
    BaseQueue::assertNotEmpty(empty());
    return queue_.front();
}

void FCFSQueue::push(const Packet& packet) {
    queue_.push_back(packet);
}
