
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin/example)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17 -g")


link_directories(${CMAKE_BINARY_DIR}/lib)

add_executable(request_get request_get.cc)
target_link_libraries(request_get
    bbt_pollevent
    bbt_http
)

add_executable(server server.cc)
target_link_libraries(server
    bbt_pollevent
    bbt_http
)