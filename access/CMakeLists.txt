file(GLOB HEADER_LIST "include/**/*.h[pp]")
file(GLOB IMPL_LIST "src/*.cpp")

add_library(accessLib ${IMPL_LIST} ${HEADER_LIST})
target_include_directories(accessLib PUBLIC "include")
target_link_libraries(accessLib PUBLIC hrlzLib rlzLib common)
