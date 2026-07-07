add_library(fse OBJECT bench.c fileio.c xxhash.c
        zlibh.c ../lib/entropy_common.c ../lib/hist.c ../lib/fse_decompress.c
        ../lib/fse_compress.c ../lib/fseU16.c ../lib/huf_compress.c ../lib/huf_decompress.c
        ../../Encoders/Encoder.cpp HuffmanFile.cpp)
target_compile_options(fse PRIVATE  -Wall -Wextra -Wcast-qual
        -Wcast-align -Wshadow -Wstrict-aliasing=1 -Wswitch-enum
        -Wundef)
target_include_directories(fse PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../lib")
target_include_directories(fse PUBLIC "${CMAKE_CURRENT_LIST_DIR}")
target_link_libraries(fse PUBLIC Encoders)
target_compile_definitions(fse PUBLIC -DDIR_FSE="${CMAKE_CURRENT_BINARY_DIR}")

add_executable(fse_exec commandline.c $<TARGET_OBJECTS:fse>)
target_compile_options(fse_exec PRIVATE  -Wall -Wextra -Wcast-qual
        -Wcast-align -Wshadow -Wstrict-aliasing=1 -Wswitch-enum
        -Wundef)
target_link_libraries(fse_exec fse)

add_executable(test_huffman test_huffman.cpp $<TARGET_OBJECTS:fse>)
target_link_libraries(test_huffman fse)

add_executable(fse_hrlz fse_hrlz.cpp $<TARGET_OBJECTS:fse>)
target_compile_options(fse_hrlz PRIVATE  -Wall -Wextra -Wcast-qual
        -Wcast-align -Wshadow -Wstrict-aliasing=1 -Wswitch-enum
        -Wundef)
target_link_libraries(fse_hrlz fse)