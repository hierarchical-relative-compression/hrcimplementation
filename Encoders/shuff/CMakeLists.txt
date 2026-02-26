set_source_files_properties(driver.c encode.c bitio.c inplace.c decode.c freq.c mysort.c nqsort.c interp.c PROPERTIES LANGUAGE CXX )

add_library(shuff_lib OBJECT driver.c encode.c bitio.c inplace.c decode.c freq.c mysort.c nqsort.c interp.c)
target_compile_definitions(shuff_lib PUBLIC -DMAX_SYMBOL_NUMBER=134217726 -DLOG2_MAX_SYMBOL_NUMBER=27
                -DDEFAULT_BLOCK_LEN=131072 -DLINUX)
target_compile_options(shuff_lib PRIVATE "-ansi" "-Wall" "-pedantic" "-O6")
target_compile_definitions(shuff_lib INTERFACE -DDIR_SHUFF="${CMAKE_CURRENT_BINARY_DIR}")
target_include_directories(shuff_lib PUBLIC ".")
target_include_directories(shuff_lib PRIVATE "..")

add_executable(shuff main_shuff.cpp $<TARGET_OBJECTS:shuff_lib>)
target_link_libraries(shuff PRIVATE shuff_lib)

add_executable(shuff_fasta main_fasta.cpp $<TARGET_OBJECTS:shuff_lib>)
target_link_libraries(shuff_fasta PRIVATE shuff_lib)