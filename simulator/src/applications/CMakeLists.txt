# Build static library
include_directories(.)
add_library(simulator_applications STATIC
    application_factory.cpp
    application.cpp
    echo.cpp
    iid_job_sizes.cpp
    tcp_reassembly.cpp
)

target_link_libraries(simulator_applications common)
