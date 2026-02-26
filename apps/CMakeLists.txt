add_executable(cluster cluster.cpp)

target_link_libraries(cluster PRIVATE hrlzLib)

add_executable(gensa gensa.cpp)

target_link_libraries(gensa PRIVATE rlzLib divsufsortLib)

add_executable(rlz rlz.cpp)

target_link_libraries(rlz PRIVATE rlzLib)

add_executable(arborescence arborescence.cpp)

target_link_libraries(arborescence PRIVATE hrlzLib)

add_executable(hrlz hrlz.cpp)

target_link_libraries(hrlz PRIVATE hrlzLib rlzLib)
target_compile_options(hrlz PRIVATE "-g")
target_link_libraries(hrlz PRIVATE utils_test)
target_link_libraries(hrlz PRIVATE utils)
target_link_libraries(hrlz PRIVATE fse)

#add_executable(rehrlz recompress.cpp)

#target_link_libraries(rehrlz PRIVATE hrlzLib rlzLib)

add_executable(access access.cpp)

target_link_libraries(access PRIVATE accessLib)

#Max: Make gdc2 executable
add_executable(gdc2_exec gdc2.cpp)
#Link it to the header
target_link_libraries(gdc2_exec PRIVATE GDC2)

add_executable(hirgc_max_exec hirgc_max_exec.cpp)
target_link_libraries(hirgc_max_exec HiRGC)
target_link_libraries(hirgc_max_exec fse)

add_executable(iDoCompMaxExec iDoCompMaxExec.cpp)
target_link_libraries(iDoCompMaxExec iDoCompMaxLib fse)

#Random tests
add_executable(test_function test_function.cpp)
target_link_libraries(test_function hrlzLib utils)

#Make compresss_hierarchy executable
add_executable(compress_hierarchy compress_hierarchy.cpp)
target_link_libraries(compress_hierarchy PRIVATE hrlzLib rlzLib)


add_executable(Array64ToArray32 Array64ToArray32.cpp)

add_executable(largest_path largest_path.cpp)
target_link_libraries(largest_path PRIVATE hrlzLib)

add_executable(rlz_exec rlz_exec.cpp)
target_link_libraries(rlz_exec PRIVATE rlzLib Encoders)