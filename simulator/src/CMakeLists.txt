# Sources
include_directories(.)
add_executable(simulator
    simulator.cpp
)

# Subdirs
add_subdirectory(packet)
add_subdirectory(applications)
add_subdirectory(queueing)
add_subdirectory(server)
add_subdirectory(traffic)

# Link libraries
target_link_libraries(simulator common)
target_link_libraries(simulator distributions)
target_link_libraries(simulator simulator_packet)
target_link_libraries(simulator simulator_applications)
target_link_libraries(simulator simulator_queueing)
target_link_libraries(simulator simulator_server)
target_link_libraries(simulator simulator_traffic)
