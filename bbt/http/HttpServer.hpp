#pragma once
#include <unordered_map>
#include <atomic>
#include <bbt/http/detail/Define.hpp>
#include <bbt/pollevent/EvThread.hpp>
#include <bbt/http/detail/Context.hpp>



namespace bbt::http
{

class HttpServer:
    public std::enable_shared_from_this<HttpServer>
{
public:
    typedef std::function<void(std::shared_ptr<detail::Context> resp)> ReqHandler;

    HttpServer();
    ~HttpServer();

    core::errcode::ErrOpt   RunInEvThread(pollevent::EvThread& evthread, const std::string& ip, short port);

    core::errcode::ErrOpt   Route(const std::string& uri, const ReqHandler& handle);
    core::errcode::ErrOpt   AsyncReply(std::shared_ptr<detail::Context> resp);

    void                    SetErrCallback(const OnErrorCallback& on_err);
protected:
    core::errcode::ErrOpt   _BindAddress(const std::string& ip, short port);
    void                    _ProcessRequest(evhttp_request* req);
    void                    _ProcessSendReply();

    static void             OnRequest(evhttp_request* req, void* arg);
private:
    struct OnReqHandle {
        std::weak_ptr<HttpServer> server;
        ReqHandler handle;
    };

    std::mutex              m_all_opt_mtx;
    // event_base*             m_io_ctx{NULL};
    evhttp*                 m_http_server{NULL};
    std::atomic_bool        m_is_running{false};
    OnErrorCallback         m_onerr{nullptr};
    std::shared_ptr<pollevent::Event> m_send_reply_event{nullptr};

    std::unordered_map<std::string, OnReqHandle*> m_handles;
    std::queue<std::shared_ptr<detail::Context>> m_async_send_response_queue;    // 异步发送响应队列
    
};
} // namespace bbt::http