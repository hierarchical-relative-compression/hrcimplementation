cmake_minimum_required(VERSION 3.21)

#GDC2 sources
add_library(GDC2 c1stage.cpp c2stage.cpp fasta.cpp hasher.cpp main.cpp p1stage.cpp qsmodel.cpp queue.cpp rangecod.cpp timer.cpp)
#Compile options for GDC2 sources
target_compile_options(GDC2 PRIVATE -Wall -O3 -m64)
#Specify include directories
target_include_directories(GDC2 INTERFACE "include")

#Specify that this library depends on zip, math and pthreads
target_link_libraries(GDC2 PUBLIC -lz -lm -lpthread)

#Import static library
add_library(ASM STATIC IMPORTED)
set_property(TARGET ASM PROPERTY IMPORTED_LOCATION ${CMAKE_CURRENT_SOURCE_DIR}/../libs/libaelf64.a)

#Specify that GDC2 depends on ASM to compile
target_link_libraries(GDC2 PUBLIC ASM)

#That should be it