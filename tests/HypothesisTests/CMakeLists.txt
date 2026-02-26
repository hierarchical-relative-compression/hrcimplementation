#Assumes a top level CMakeLists.txt includes these tests.
#If that is not the case, uncomment the following 6 lines:
#cmake_minimum_required(VERSION 3.21)
#set(CMAKE_C_COMPILER "gcc")
#set(CMAKE_CXX_COMPILER "g++")
#project(PBTestsHypothesis C CXX)
#set(CMAKE_CXX_STANDARD 20)
#set(CMAKE_CXX_STANDARD_REQUIRED True)

#Sources for the engine
add_executable(main_pbtests CppEngine/main_pbtests.cpp CppEngine/json_parse.cpp CppEngine/json.hpp CppEngine/json_parse.hpp)
#target_compile_definitions(main_pbtests PUBLIC -DNDEBUG)

#Any further CPP sources must be specified in CppTests/CMakeLists.txt
add_subdirectory(CppTests)