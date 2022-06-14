cmake_minimum_required(VERSION 3.10)
project(SurgeProtector)

# Output targets
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Boost
find_package(Boost REQUIRED COMPONENTS program_options)
link_libraries(Boost::program_options)

# Libconfig
find_library(LIBCONFIG config)
find_library(LIBCONFIGPP config++)

if(NOT LIBCONFIG OR NOT LIBCONFIGPP)
    message(FATAL_ERROR "libconfig not found")
else()
    message(STATUS "Found libconfig: " ${LIBCONFIG})
    message(STATUS "Found libconfig++: " ${LIBCONFIGPP})
endif()

link_libraries(${LIBCONFIG})
link_libraries(${LIBCONFIGPP})

# C++ STD version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# C STD version
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_C_EXTENSIONS True)

# CXX flags
add_compile_options(-m64 -O3 -march=native -g
                    -pedantic-errors -Wall -Wextra -Werror)

# Headers
include_directories(.)

# Sources
add_subdirectory(common)
add_subdirectory(scheduler)
add_subdirectory(simulator)
