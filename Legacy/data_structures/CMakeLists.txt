add_library(PartialSums ../Legacy/data_structures/FenwickTree.cpp ../Legacy/data_structures/PartialSums.hpp)
target_include_directories(PartialSums PUBLIC ".")
set_property(TARGET PartialSums PROPERTY POSITION_INDEPENDENT_CODE ON)