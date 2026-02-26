add_library(iDoCompLib idc_generate_mapping.c main_iDoComp.cpp stats.c arith.c fasta_decompressor.c idc_load_chr.c os_stream.c
        fasta_compressor.c sam_stream.c)
target_link_libraries(iDoCompLib PUBLIC -lm)
#Specify include directories
target_include_directories(iDoCompLib INTERFACE "include")
#Specify PIC so that it can be used in a shared library
set_property(TARGET iDoCompLib PROPERTY POSITION_INDEPENDENT_CODE ON)