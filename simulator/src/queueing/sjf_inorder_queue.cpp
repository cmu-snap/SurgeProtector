#include "sjf_inorder_queue.h"

/**
 * SJFInorderFlowMetadata implementation.
 */
SJFInorderFlowMetadata&
SJFInorderFlowMetadata::push(const Packet& packet) {
    // Update the flow's cumulative job size, and
    // insert the given packet into the queue.
    queue_.push(packet);
    total_jsize_ += packet.getJobSizeEstimate();

    return *this;
}

Packet SJFInorderFlowMetadata::pop() {
    if (queue_.empty()) {
        throw std::runtime_error("Cannot pop an empty flow queue.");
    }
    // Update the flow queue state
    Packet packet = queue_.front();
    total_jsize_ -= packet.getJobSizeEstimate();
    assert(total_jsize_ >= 0); // Sanity check
    queue_.pop();

    return packet;
}

/**
 * SJFInorderQueue implementation.
 */
Packet SJFInorderQueue::pop() {
    BaseQueue::assertNotEmpty(empty());
    const FlowId flow_id = priorities_.top().tag();
    auto iter = data_.find(flow_id); // Fetch metadata
    assert(iter != data_.end() && !iter->second.empty());
    const Packet packet = iter->second.pop(); // Pop the flow queue

    // If the queue is not empty, update the correspoding
    // flow priority in the heap using the stored handle.
    if (!iter->second.empty()) {
        priorities_.update(
            iter->second.getHandle(),
            SJFPriorityEntry(flow_id, iter->second.getFlowRatio())
        );
    }
    // Else, purge both the flow mapping and heap entry
    else {
        priorities_.pop();
        data_.erase(iter);
    }

    size_--; // Update the global queue size
    return packet;
}

Packet SJFInorderQueue::peek() const {
    BaseQueue::assertNotEmpty(empty());

    const FlowId flow_id = priorities_.top().tag();
    return data_.at(flow_id).front();
}

void SJFInorderQueue::push(const Packet& packet) {
    // Insert this packet into the flow queue. If this is the
    // HoL packet for this flow, also insert it into the heap.
    const FlowId flow_id = packet.getFlowId();
    auto iter = data_.find(flow_id);
    if (iter == data_.end()) {

        SJFInorderFlowMetadata& data = data_[flow_id].push(packet);
        assert(data.size() == 1); // Sanity check
        handle_t handle = priorities_.push(
            SJFPriorityEntry(flow_id, data.getFlowRatio())
        );
        // Set the flow's heap handle
        data.setHandle(handle);
    }
    else {
        assert(!iter->second.empty()); // Sanity check
        SJFInorderFlowMetadata data = iter->second.push(packet);
        priorities_.update(
            iter->second.getHandle(),
            SJFPriorityEntry(flow_id, data.getFlowRatio())
        );
    }
    size_++; // Update the global queue size
}
