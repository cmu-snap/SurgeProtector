#include "queue_factory.h"

// Library headers
#include "fcfs_queue.h"
#include "fq_queue.h"
#include "sjf_queue.h"
#include "sjf_inorder_queue.h"
#include "wsjf_queue.h"
#include "wsjf_inorder_queue.h"

// STD headers
#include <stdexcept>

BaseQueue* QueueFactory::
generate(const libconfig::Setting& queue_config) {
    BaseQueue* queue = nullptr;
    std::string policy; // Queueing policy
    if (!queue_config.lookupValue("policy", policy)) {
        throw std::runtime_error("Must specify 'policy' to use.");
    }
    // FCFS queue
    else if (policy == FCFSQueue::name()) {
        queue = new FCFSQueue();
    }
    // FQ queue
    else if (policy == FQQueue::name()) {
        queue = new FQQueue();
    }
    // SJF queue
    else if (policy == SJFQueue::name()) {
        queue = new SJFQueue();
    }
    // SJF-Inorder queue
    else if (policy == SJFInorderQueue::name()) {
        queue = new SJFInorderQueue();
    }
    // WSJF queue
    else if (policy == WSJFQueue::name()) {
        queue = new WSJFQueue();
    }
    // WSJF-Inorder queue
    else if (policy == WSJFInorderQueue::name()) {
        queue = new WSJFInorderQueue();
    }
    // Unknown policy
    else {
        throw std::runtime_error(
            "Unknown queueing policy: " + policy + ".");
    }
    return queue;
}
