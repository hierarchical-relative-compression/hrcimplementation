file(GLOB HEADER_LIST "include/**/*.h[pp]")
file(GLOB IMPL_LIST "src/*.cpp")

#add_library(common ${IMPL_LIST} ${HEADER_LIST})
add_library(common INTERFACE)
target_link_libraries(common INTERFACE utils)
target_include_directories(common INTERFACE "include")
