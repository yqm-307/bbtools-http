cmake_minimum_required(VERSION 3.1)

project(bbtools-http CXX)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g")

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib) 
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
include_directories(
    ${PROJECT_SOURCE_DIR}/bbt
    ${PROJECT_SOURCE_DIR}
)

file(GLOB_RECURSE SRCS 
    ${PROJECT_SOURCE_DIR}/bbt/*.cc
    ${PROJECT_SOURCE_DIR}/bbt/*.hpp
)

add_library(bbtools-http SHARED ${SRCS})
target_link_libraries(bbtools-http
    curl
    event_extra
    event_core
    pthread
    bbt_core
    bbt_coroutine
)

enable_testing()
# add_subdirectory(test)