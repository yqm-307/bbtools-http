#pragma once
#include <curl/curl.h>
#include <functional>

// 服务器接受最大协议长度，超长会被截取
#define HTTP_SERVER_PROTOCOL_MAX_LENGTH 65535

#define HTTP_SERVER_ERROR_RESP_HEAD "HTTP/1.1 500\r\n"
#define HTTP_SERVER_OK_RESP_HEAD    "HTTP/1.1 200 OK\r\n"