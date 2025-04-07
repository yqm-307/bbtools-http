#pragma once
#include <functional>
#include <curl/curl.h>
#include <bbt/core/buffer/Buffer.hpp>
#include <bbt/core/errcode/Errcode.hpp>

namespace bbt::http
{

// 服务器接受最大协议长度，超长会被截取
#define HTTP_SERVER_PROTOCOL_MAX_LENGTH 65535
#define UPDATE_TIME_MS 50

#define HTTP_SERVER_REQUEST_TIMEOUT_MS  1000

#define HTTP_SERVER_ERROR_RESP_HEAD "HTTP/1.1 500\r\n"
#define HTTP_SERVER_OK_RESP_HEAD    "HTTP/1.1 200 OK\r\n"

class HttpClient;
class HttpServer;

enum emErr : core::errcode::ErrType
{
    ERR_UNKNOWN = 0,
};

typedef int64_t RequestId;
typedef std::function<void(CURL* req, core::Buffer& body, CURLcode)> RespHandler;


} // namespace bbt::http