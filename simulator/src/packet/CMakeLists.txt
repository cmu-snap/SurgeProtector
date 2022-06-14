# Build static library
include_directories(.)
add_library(simulator_packet STATIC
    packet.cpp
)
