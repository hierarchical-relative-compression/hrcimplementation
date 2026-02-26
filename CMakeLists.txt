cmake_minimum_required(VERSION 3.21)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

project(HierarchicalRelativeLempelZiv C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -funroll-loops -DNDEBUG")

option(PROFILING "Add profiling timings to stderr" ON)
if(PROFILING)
  message(STATUS "Profiling enabled")
  add_definitions(-DPROFILING=1)
endif()

enable_testing()

add_subdirectory(FiniteStateEntropy)
add_subdirectory(Encoders)
add_subdirectory(divsufsort)
add_subdirectory(divsufsort64)
add_subdirectory(common)
add_subdirectory(hrlz)
add_subdirectory(rlz)
add_subdirectory(access)
add_subdirectory(apps)
add_subdirectory(compression_algorithms)
add_subdirectory(data_structures)
add_subdirectory(utils)
add_subdirectory(tests)