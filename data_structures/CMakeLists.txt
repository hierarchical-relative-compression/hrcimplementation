add_library(PartialSums FenwickTree.cpp PartialSums.hpp)
target_include_directories(PartialSums PUBLIC ".")
set_property(TARGET PartialSums PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(Hashing NoRehashingHashMap.hpp HashFunction.cpp HashFunction.hpp AggregatingMultimap.hpp
        NoRehashingHashSet.hpp TabulationHashing.hpp StrongUniversalHashing.hpp HiRGC64bitsChaining.hpp
        HiRGC32bitLinearProbing.hpp)
target_include_directories(Hashing PUBLIC ".")
target_link_libraries(Hashing PUBLIC utils)
set_property(TARGET Hashing PROPERTY POSITION_INDEPENDENT_CODE ON)

add_executable(testHashing testTable.cpp)
target_link_libraries(testHashing PRIVATE Hashing)