file(GLOB HEADER_LIST "include/**/*.h[pp]")
file(GLOB IMPL_LIST "src/*.cpp")

add_library(rlzLib ${IMPL_LIST} ${HEADER_LIST})
target_link_libraries(rlzLib PUBLIC common)
target_include_directories(rlzLib PUBLIC "include")
target_include_directories(rlzLib PUBLIC "${CMAKE_SOURCE_DIR}/hrlz/include/hrlz")
