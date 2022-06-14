# Build static library
include_directories(.)
add_library(common STATIC
    tsc_clock.cpp
    utils.cpp
)

# Sources
add_subdirectory(distributions)
