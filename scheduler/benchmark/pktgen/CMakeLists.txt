# DPDK
include_directories(${dpdk_INCLUDE_DIRS})
set(RTE_TARGET x86_64-native-linuxapp-gcc)

# Sources
add_executable(sched_benchmark_pktgen
    pktgen.cpp
)

# Link libraries
target_link_libraries(sched_benchmark_pktgen common)
target_link_libraries(sched_benchmark_pktgen distributions)
target_link_libraries(sched_benchmark_pktgen ${dpdk_LIBRARIES})
