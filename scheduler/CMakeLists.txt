# DPDK
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
find_package(dpdk REQUIRED)

# Headers
include_directories(.)

# Sources
add_subdirectory(benchmark)
