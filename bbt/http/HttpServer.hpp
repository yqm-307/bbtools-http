#pragma once
#include <unordered_map>
#include <atomic>
#include <bbt/http/detail/Define.hpp>
#include <event2/http.h>
#include <event2/buffer.h>

namespace bbt::http
{

class HttpServer
{
    friend void EventHttpRequest(evhttp_request* req, void* arg);
public:
    typedef std::function<void(RequestId, core::Buffer&, HttpServer*)> ReqHandler;

    HttpServer(event_base* ev);
    ~HttpServer();

    core::errcode::ErrOpt BindListenFd(const std::string& ip, short port);
    core::errcode::ErrOpt SetHandler(const std::string& path, ReqHandler cb);
    core::errcode::ErrOpt DoReply(RequestId id, int code, std::string status, const core::Buffer& buf);
protected:
    core::errcode::ErrOpt __BindAddress(const std::string& ip, short port);
    core::errcode::ErrOpt __AddHandler(const std::string& uri);
    core::errcode::ErrOpt __DelHandler(const std::string& uri);

    core::errcode::ErrOpt __Handler(evhttp_request* req); 
    core::errcode::ErrTuple<RequestId> __OnRequest(evhttp_request* req);
private:
    event_base*             m_io_ctx{NULL};
    evhttp*                 m_http_server{NULL};
    bool                    m_is_running{false};
    static std::atomic_uint64_t    s_request_id;

    std::unordered_map<std::string, ReqHandler> m_handles;
    /* 等待返回的请求 */
    std::unordered_map<RequestId, evhttp_request*> m_wait_requests;
};
} // namespace bbt::http