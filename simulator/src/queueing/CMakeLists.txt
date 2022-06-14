# Build static library
include_directories(.)
add_library(simulator_queueing STATIC
    fcfs_queue.cpp
    fq_queue.cpp
    queue_factory.cpp
    sjf_queue.cpp
    sjf_inorder_queue.cpp
    wsjf_queue.cpp
    wsjf_inorder_queue.cpp
)

target_link_libraries(simulator_queueing common)
