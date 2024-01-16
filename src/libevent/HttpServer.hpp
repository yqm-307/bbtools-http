#pragma once
#include <memory>
#include <unordered_map>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <bbt/buffer/Buffer.hpp>
#include "detail/Define.hpp"


namespace bbt::http::ev
{

class HttpServer
{
    friend void EventHttpRequest(evhttp_request* req, void* arg);
public:
    typedef std::function<std::tuple<int, std::string, buffer::Buffer>(buffer::Buffer&, HttpServer*)> ReqHandler;

    HttpServer(event_base* ev);
    ~HttpServer();

    ErrOpt BindListenFd(const std::string& ip, short port);
    ErrOpt SetHandler(const std::string& path, ReqHandler cb);
protected:
    ErrOpt __BindAddress(const std::string& ip, short port);
    ErrOpt __AddHandler(const std::string& uri);
    ErrOpt __DelHandler(const std::string& uri);

    ErrOpt __Handler(evhttp_request* req); 
private:
    event_base*             m_io_ctx{NULL};
    evhttp*                 m_http_server{NULL};
    bool                    m_is_running{false};

    std::unordered_map<std::string, ReqHandler> m_handles;
};

}