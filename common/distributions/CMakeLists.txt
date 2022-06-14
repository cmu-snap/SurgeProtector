# Build static library
include_directories(.)
add_library(distributions STATIC
    constant_distribution.cpp
    distribution.cpp
    distribution_factory.cpp
    exponential_distribution.cpp
    normal_distribution.cpp
    uniform_distribution.cpp
)
