#pragma once
#include "./Define.hpp"
#include "engine/Errcode.hpp"

namespace bbt::http
{

class IHttpServer
{
public:
    typedef std::function<void(const char*, size_t)> HttpHandler;

    // virtual ErrOpt SetHandler(const char* path, HttpHandler) = 0;
    // virtual ErrOpt SendReply(const char* resp, size_t len) = 0;


};

}