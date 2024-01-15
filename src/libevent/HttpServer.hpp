#pragma once
#include <memory>
#include <unordered_map>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include "engine/IHttpServer.hpp"


namespace bbt::http::ev
{

class HttpServer;

struct EventHttpRequestPrvData
{
    std::weak_ptr<HttpServer> m_wptr;
};

class HttpServer:
    public IHttpServer,
    public std::enable_shared_from_this<HttpServer>
{
    friend void EventHttpRequest(evhttp_request* req, void* arg);
public:
    typedef std::function<void(evhttp_request*, HttpServer*)> ReqHandler;

    HttpServer(event_base* ev);
    ~HttpServer();

    ErrOpt Listen(const std::string& ip, short port);
    ErrOpt SetHandler(const std::string& path, ReqHandler cb);
protected:
    ErrOpt __Listen();
    ErrOpt __AddHandler(const std::string& uri);
    ErrOpt __DelHandler(const std::string& uri);

    void __Handler(evhttp_request* req); 
private:
    event_base*         m_io_ctx{NULL};
    event*              m_req_handler{NULL};
    evhttp*             m_http_server{NULL};
    evhttp_bound_socket* m_http_socket{NULL};

    EventHttpRequestPrvData* m_prvdata{NULL};
    std::unordered_map<std::string, ReqHandler> m_handles;
};

}