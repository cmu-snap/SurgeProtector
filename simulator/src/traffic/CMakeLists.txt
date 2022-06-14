# Build static library
include_directories(.)
add_library(simulator_traffic STATIC
    synthetic_trafficgen.cpp
    trafficgen_factory.cpp
    trace_trafficgen.cpp
    trafficgen.cpp
)

target_link_libraries(simulator_traffic common)
target_link_libraries(simulator_traffic distributions)
