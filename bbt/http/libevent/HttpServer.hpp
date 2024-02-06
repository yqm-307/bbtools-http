#pragma once
#include <memory>
#include <unordered_map>
#include <event2/event.h>
#include <event2/http.h>
#include <event2/buffer.h>
#include <bbt/base/buffer/Buffer.hpp>

#include <bbt/http/detail/Define.hpp>


namespace bbt::http::ev
{

class HttpServer
{
    friend void EventHttpRequest(evhttp_request* req, void* arg);
public:
    typedef std::function<void(RequestId, buffer::Buffer&, HttpServer*)> ReqHandler;

    HttpServer(event_base* ev);
    ~HttpServer();

    ErrOpt BindListenFd(const std::string& ip, short port);
    ErrOpt SetHandler(const std::string& path, ReqHandler cb);
    ErrOpt DoReply(RequestId id, int code, std::string status, const buffer::Buffer& buf);
protected:
    ErrOpt __BindAddress(const std::string& ip, short port);
    ErrOpt __AddHandler(const std::string& uri);
    ErrOpt __DelHandler(const std::string& uri);

    ErrOpt __Handler(evhttp_request* req); 
    std::pair<ErrOpt, RequestId> __OnRequest(evhttp_request* req);
private:
    event_base*             m_io_ctx{NULL};
    evhttp*                 m_http_server{NULL};
    bool                    m_is_running{false};

    std::unordered_map<std::string, ReqHandler> m_handles;
    /* 等待返回的请求 */
    std::map<RequestId, evhttp_request*>    m_wait_requests;
};

}