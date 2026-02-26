add_subdirectory(HypothesisTests)

add_executable(compare_fasta compare_fasta.cpp)
#Self-contained

add_executable(test_complete_cluster test_complete_cluster.cpp)
target_link_libraries(test_complete_cluster hrlzLib)


add_test(Test_complete_cluster test_complete_cluster)
