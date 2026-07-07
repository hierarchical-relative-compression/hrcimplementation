add_executable(sa_fasta sa_generator.c sais.c)
target_link_libraries(sa_fasta PRIVATE -lm)

add_executable(sa_fasta64 sa_generator_64.c)
target_link_libraries(sa_fasta64 PRIVATE divsufsort64 divsufsortLib)

add_custom_target(link_sa_fasta ALL COMMAND ${CMAKE_COMMAND}
        -E create_symlink "${CMAKE_CURRENT_BINARY_DIR}/sa_fasta" "${CMAKE_BINARY_DIR}/apps/sa_fasta"
        COMMAND ${CMAKE_COMMAND}
        -E create_symlink "${CMAKE_CURRENT_BINARY_DIR}/sa_fasta64" "${CMAKE_BINARY_DIR}/apps/sa_fasta64")
add_dependencies(link_sa_fasta sa_fasta sa_fasta64)