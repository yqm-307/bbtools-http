#pragma once
#include <curl/curl.h>
#include <functional>

#include <bbt/uuid/EasyID.hpp>

#include "./Errcode.hpp"


// 服务器接受最大协议长度，超长会被截取
#define HTTP_SERVER_PROTOCOL_MAX_LENGTH 65535
#define UPDATE_TIME_MS 50

#define HTTP_SERVER_REQUEST_TIMEOUT_MS  1000

#define HTTP_SERVER_ERROR_RESP_HEAD "HTTP/1.1 500\r\n"
#define HTTP_SERVER_OK_RESP_HEAD    "HTTP/1.1 200 OK\r\n"

namespace bbt::http
{

enum emEasyIdDiff
{
    EM_ID_REQUEST,
};

typedef int64_t RequestId;

static inline RequestId GenerateRequestId()
{ 
    return bbt::uuid::EasyID<bbt::uuid::EM_AUTO_INCREMENT, emEasyIdDiff::EM_ID_REQUEST>::GenerateID(); 
};

};