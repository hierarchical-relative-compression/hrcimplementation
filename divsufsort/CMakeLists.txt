file(GLOB HEADER_LIST "include/**/*.h")
file(GLOB IMPL_LIST "src/*.c")

add_library(divsufsortLib ${IMPL_LIST} ${HEADER_LIST})

target_include_directories(divsufsortLib PUBLIC "include")
