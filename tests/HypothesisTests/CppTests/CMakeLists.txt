#Add all the needed sources to the engine
set(TEST_SOURCES "${CMAKE_CURRENT_LIST_DIR}/test_arith.cpp" "${CMAKE_CURRENT_LIST_DIR}/test_compress_gdc2.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_compress_hirgc_max.cpp" "${CMAKE_CURRENT_LIST_DIR}/test_compress_idocomp.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_compress_rlz.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_compress_sccg.cpp" "${CMAKE_CURRENT_LIST_DIR}/test_idocomp_max.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/utils_tests.cpp" "${CMAKE_CURRENT_LIST_DIR}/test_partial_sums.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/HRLZTests.cpp" "${CMAKE_CURRENT_LIST_DIR}/test_agg_multimap.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/test_finitestateentropy.cpp"
)
target_include_directories(main_pbtests PRIVATE "${CMAKE_CURRENT_LIST_DIR}")
target_sources(main_pbtests PRIVATE "${TEST_SOURCES}")

#Add any needed links to libraries that the tests need
target_link_libraries(main_pbtests PUBLIC hrlzLib)
target_link_libraries(main_pbtests PUBLIC iDoCompMaxLib)
target_link_libraries(main_pbtests PUBLIC rlzLib)
target_link_libraries(main_pbtests PUBLIC ArithmeticCoder)
target_link_libraries(main_pbtests PUBLIC PartialSums)
target_link_libraries(main_pbtests PUBLIC Hashing)
target_link_libraries(main_pbtests PUBLIC HiRGC)
target_link_libraries(main_pbtests PUBLIC fse)
#target_link_libraries(main_pbtests PRIVATE iDoCompLib HiRGC SCCGLib)

#Make library for utils_test because compare_fasta is absurdly useful
add_library(utils_test utils_tests.cpp)
target_include_directories(utils_test INTERFACE ".")

#Make a library with all the tests
add_library(HRLZTests HRLZTests.cpp "${TEST_SOURCES}") # test_compress_sccg.cpp
target_include_directories(HRLZTests PRIVATE ".")
#Add any needed links to libraries that the tests need
target_link_libraries(HRLZTests PRIVATE hrlzLib)
target_link_libraries(HRLZTests PUBLIC rlzLib)
target_link_libraries(HRLZTests PUBLIC HiRGC)
target_link_libraries(HRLZTests PUBLIC iDoCompMaxLib)
target_link_libraries(HRLZTests PUBLIC iDoCompLib)
target_link_libraries(HRLZTests PUBLIC ArithmeticCoder)
target_link_libraries(HRLZTests PUBLIC PartialSums)
target_link_libraries(HRLZTests PUBLIC fse)
#Add public header
#set_target_properties(HRLZTests PROPERTIES PUBLIC_HEADER HRLZTests.h)
#target_include_directories(HRLZTests PRIVATE "../CppEngine/")

#Add executable to test the tests
add_executable(test_of_tests tests_of_tests.cpp)
target_include_directories(test_of_tests PRIVATE ".")
target_link_libraries(test_of_tests PRIVATE HRLZTests)
