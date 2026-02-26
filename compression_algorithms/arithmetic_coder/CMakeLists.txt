add_library(ArithmeticCoder AdaptiveScalingCoder.cpp)#Specify include directories
target_include_directories(ArithmeticCoder INTERFACE ".")
target_link_libraries(ArithmeticCoder PUBLIC PartialSums)
#Specify PIC so that it can be used in a shared library
set_property(TARGET ArithmeticCoder PROPERTY POSITION_INDEPENDENT_CODE ON)
add_subdirectory(BitUtils)