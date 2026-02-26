add_library(iDoCompMaxLib iDoCompMaxMain.cpp matching_rlz.cpp iDoCompMaxPostprocessing.cpp)
target_link_libraries(iDoCompMaxLib PRIVATE ArithmeticCoder utils fse)
target_link_libraries(iDoCompMaxLib PRIVATE common)
#Specify include directories
target_include_directories(iDoCompMaxLib INTERFACE "include")
target_compile_definitions(iDoCompMaxLib PRIVATE -DDIR_7ZA="${CMAKE_CURRENT_LIST_DIR}")
target_include_directories(iDoCompMaxLib PUBLIC "${CMAKE_SOURCE_DIR}/hrlz/include/hrlz")
#Specify PIC so that it can be used in a shared library
set_property(TARGET iDoCompMaxLib PROPERTY POSITION_INDEPENDENT_CODE ON)
find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    target_link_libraries(iDoCompMaxLib PUBLIC OpenMP::OpenMP_CXX)
ELSE ()
    MESSAGE(STATUS "Could not locate OpenMP.")
endif()


add_subdirectory(sais_lite)