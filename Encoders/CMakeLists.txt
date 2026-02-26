add_subdirectory(shuff)
add_library(Encoders STATIC Encoder.cpp ShuffEncoder.cpp)
target_include_directories(Encoders PUBLIC ".")
target_link_libraries(Encoders PRIVATE shuff_lib)