cmake_minimum_required(VERSION 3.21)

#We create a custom target and command to compile the java project
set(SCCG_OUTPUT_DIR ${CMAKE_BINARY_DIR}/apps)

add_custom_command(
        OUTPUT ${PROJECT_BINARY_DIR}/apps/SCCGConnector.class
        COMMAND javac -cp ${CMAKE_SOURCE_DIR}/compression_algorithms/SCCGLib ${CMAKE_SOURCE_DIR}/compression_algorithms/SCCGLib/SCCG/ORGC.java -d ${SCCG_OUTPUT_DIR}
        COMMAND javac -cp ${CMAKE_SOURCE_DIR}/compression_algorithms/SCCGLib ${CMAKE_SOURCE_DIR}/compression_algorithms/SCCGLib/SCCG/ORGD.java -d ${SCCG_OUTPUT_DIR}
        COMMAND javac -cp ${CMAKE_SOURCE_DIR}/compression_algorithms/SCCGLib ${CMAKE_SOURCE_DIR}/compression_algorithms/SCCGLib/SCCG/SCCGC.java -d ${SCCG_OUTPUT_DIR}
        COMMAND javac -cp ${CMAKE_SOURCE_DIR}/compression_algorithms/SCCGLib ${CMAKE_SOURCE_DIR}/compression_algorithms/SCCGLib/SCCG/SCCGD.java -d ${SCCG_OUTPUT_DIR}
        COMMAND javac -cp ${CMAKE_SOURCE_DIR}/compression_algorithms/SCCGLib ${CMAKE_SOURCE_DIR}/compression_algorithms/SCCGLib/SCCGConnector.java -d ${SCCG_OUTPUT_DIR}
        VERBATIM
)
#The pwd on the custom target is the build directory /compression_algorithms_SCCGLib
add_custom_target(JavaSCCG
        DEPENDS ${PROJECT_BINARY_DIR}/apps/SCCGConnector.class
)

#Library with sources
add_library(SCCGLib STATIC testPipe.cpp)
#Add compile definition so that it knows what directory it is in, and can find the java
target_compile_definitions(SCCGLib PUBLIC -DTHIS_DIR_STR="${SCCG_OUTPUT_DIR}")
#Declare target-level dependency to the java project
add_dependencies(SCCGLib JavaSCCG)
#Anybody who links to SCCGLib inherits these includes
target_include_directories(SCCGLib INTERFACE "include")