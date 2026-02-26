cmake_minimum_required(VERSION 3.21)

#Library with sources
add_library(HiRGC STATIC
        hirgc_max.cpp
        include/hirgc/hirgc_max.hpp
        de_hirgc_max.cpp "${CMAKE_SOURCE_DIR}/hrlz/include/hrlz/RefDataStructure.hpp")
target_link_libraries(HiRGC PRIVATE ArithmeticCoder utils fse)
target_link_libraries(HiRGC PUBLIC Hashing)
target_link_libraries(HiRGC PRIVATE common)
target_include_directories(HiRGC PUBLIC "${CMAKE_SOURCE_DIR}/hrlz/include/hrlz")
#Specify headers that any library that links to this should have
target_include_directories(HiRGC INTERFACE "include")
#Specify PIC so that it can be used in a shared library
set_property(TARGET HiRGC PROPERTY POSITION_INDEPENDENT_CODE ON)
#Add the 7za directory as a compile definition
target_compile_definitions(HiRGC PUBLIC -DDIR_7ZA="${CMAKE_CURRENT_LIST_DIR}")
