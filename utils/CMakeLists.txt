#Utils lib for misc stuff

add_library(utils PrettyPrint.cpp GlobalRandom.cpp)
target_include_directories(utils PUBLIC "include")
set_property(TARGET utils PROPERTY POSITION_INDEPENDENT_CODE ON)