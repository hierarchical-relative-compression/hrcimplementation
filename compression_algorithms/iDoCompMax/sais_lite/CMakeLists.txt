add_executable(sa_fasta sa_generator.c sais.c)
target_link_libraries(sa_fasta PRIVATE -lm)

add_executable(sa_fasta64 sa_generator_64.c)
target_link_libraries(sa_fasta64 PRIVATE divsufsort64 divsufsortLib)