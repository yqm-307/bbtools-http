
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin/test)

include_directories(
    ${PROJECT_SOURCE_DIR}/src
)


link_libraries(${PROJECT_BINARY_DIR}/lib)

add_executable(http_server
    ${PROJECT_SOURCE_DIR}/test/http_server.cc
)

target_link_libraries(http_server
    ybbt_http
    curl
    event_core
)

add_test(NAME http_server COMMAND http_server)

link_libraries(${PROJECT_BINARY_DIR}/lib)

add_executable(http_client
    ${PROJECT_SOURCE_DIR}/test/http_client.cc
)

target_link_libraries(http_client
    ybbt_http
    curl
    event_core
)

add_test(NAME http_client COMMAND http_client)