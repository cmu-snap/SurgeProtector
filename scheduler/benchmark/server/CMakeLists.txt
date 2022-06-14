# DPDK
include_directories(${dpdk_INCLUDE_DIRS})
set(RTE_TARGET x86_64-native-linuxapp-gcc)

# Sources
include_directories(.)
add_executable(sched_benchmark_server
    server.cpp
)

# Subdirectories
add_subdirectory(policies)

# Link libraries
target_link_libraries(sched_benchmark_server common)
target_link_libraries(sched_benchmark_server sched_policies)
target_link_libraries(sched_benchmark_server ${dpdk_LIBRARIES})
