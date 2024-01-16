cmake_minimum_required(VERSION 3.1)

project(httphelper CXX)


set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g -O2")

set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib) 
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
include_directories(
    ${PROJECT_SOURCE_DIR}/src
)

file(GLOB_RECURSE SRCS 
    ${PROJECT_SOURCE_DIR}/src/*.cc
    ${PROJECT_SOURCE_DIR}/src/*.hpp
)

add_library(ybbt_http SHARED ${SRCS})
target_link_libraries(ybbt_http
    curl
    event_extra
    event_core
    pthread
    ybbt
)
enable_testing()
add_subdirectory(test)