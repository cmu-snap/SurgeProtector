# Build static library
include_directories(.)
add_library(sched_policies STATIC
    policy_fcfs.cpp
    policy_wsjf_dropmax.cpp
    policy_wsjf_droptail.cpp
    policy_wsjf_hffs.cpp
)

target_link_libraries(sched_policies ${dpdk_LIBRARIES})
