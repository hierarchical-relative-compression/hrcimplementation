file(GLOB HEADER_LIST "include/**/*.h[pp]")
file(GLOB IMPL_LIST "src/*.cpp")

add_library(hrlzLib ${IMPL_LIST} ${HEADER_LIST})
target_include_directories(hrlzLib PUBLIC "include")
target_link_libraries(hrlzLib PUBLIC rlzLib )
target_link_libraries(hrlzLib PUBLIC common) #Also includes utils
target_link_libraries(hrlzLib PUBLIC GDC2)
target_link_libraries(hrlzLib PUBLIC HiRGC)
target_link_libraries(hrlzLib PUBLIC SCCGLib)
target_link_libraries(hrlzLib PUBLIC iDoCompLib)
target_link_libraries(hrlzLib PUBLIC iDoCompMaxLib)
target_link_libraries(hrlzLib PRIVATE utils_test)
target_link_libraries(hrlzLib PUBLIC fse)
target_link_libraries(hrlzLib PRIVATE Encoders)
#Add compile definition so that we can know in what directory the binaries are stored
#and we can call f.ex. GDC2's binary
set(APP_OUTPUT_DIR ${CMAKE_BINARY_DIR}/apps)
target_compile_definitions(hrlzLib PUBLIC -DAPP_DIR_STR="${APP_OUTPUT_DIR}")

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
  target_link_libraries(hrlzLib PUBLIC OpenMP::OpenMP_CXX)
ELSE ()
  MESSAGE(STATUS "Could not locate OpenMP.")
endif()
