#pragma once
#include "./Define.hpp"
#include "./Errcode.hpp"

namespace bbt::http
{

enum HttpRequestResult
{
};

class IHttpClient
{
public:
    typedef std::function<void(HttpRequestResult , const char*, size_t)> HttpHandler;

    /**
     * @brief 向url发起 http request
     * 
     * @param url 
     * @param body 
     * @param do_resp_handler 
     */
    virtual ErrOpt Request(const char* url, const char* body, HttpHandler do_resp_handler) = 0;

    /**
     * @brief 向指定ip、port发起 http request
     * 
     * @param ip 
     * @param port 
     * @param body 
     * @param do_resp_handler 
     */
    virtual ErrOpt Request(const char* ip, short port, const char* body, HttpHandler do_resp_handler) = 0;
};

}