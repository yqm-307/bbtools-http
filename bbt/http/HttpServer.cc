#include <cstring>
#include <event2/keyvalq_struct.h>

#include <bbt/core/util/Assert.hpp>
#include <bbt/pollevent/Event.hpp>
#include <bbt/pollevent/EventLoop.hpp>
#include <bbt/http/HttpServer.hpp>

#define ERR_PREFIX BBT_HTTP_MODULE_NAME "[HttpServer]"

using namespace bbt::core;
using namespace bbt::core::errcode;


namespace bbt::http
{

void HttpServer::OnRequest(evhttp_request* req, void* arg)
{
    OnReqHandle* handle = (OnReqHandle*)arg;

    if (handle->handle)
        handle->handle(std::make_shared<detail::Context>(handle->server, req));
}


Buffer Req2Buffer(evhttp_request* req)
{
    Buffer ybuf;
    evbuffer* evbuf = evhttp_request_get_input_buffer(req);
    static char recv_buf[4096];

    while (evbuffer_get_length(evbuf) > 0) {
        int n = 0;
        n = evbuffer_remove(evbuf, recv_buf, sizeof(recv_buf));
        if (n > 0)
            ybuf.WriteString(recv_buf, n);
    }

    return ybuf;
}

Buffer Req2Header(evhttp_request* req)
{
    Buffer ybuf;
    evkeyvalq* headers = evhttp_request_get_input_headers(req);

    for (evkeyval* p = headers->tqh_first; p != NULL; p = p->next.tqe_next)
    {
        ybuf.WriteString(p->key, strlen(p->key));
        ybuf.WriteString(":", 1);
        ybuf.WriteString(p->value, strlen(p->value));
        ybuf.WriteString("\r\n", 2);
    }
    
    return ybuf;
}

HttpServer::HttpServer()
{
}

HttpServer::~HttpServer()
{
    std::lock_guard<std::mutex> guard(m_all_opt_mtx);

    for (auto&& [uri, onreq_handle] : m_handles)
    {
        delete onreq_handle;
    }

    if (m_send_reply_event != nullptr) {
        m_send_reply_event->CancelListen();
        m_send_reply_event = nullptr;
    }

    if (m_http_server != nullptr) {
        evhttp_free(m_http_server);
        m_http_server = nullptr;
    }
    
}


ErrOpt HttpServer::_BindAddress(const std::string& ip, short port)
{
    evhttp_bound_socket* m_listen_fd = evhttp_bind_socket_with_handle(m_http_server, ip.c_str(), port);

    if (m_listen_fd == nullptr) {
        return Errcode("evhttp_bind_socket_with_handle() failed!", emErr::ERR_UNKNOWN);
    }

    return std::nullopt;
}

ErrOpt HttpServer::RunInEvThread(pollevent::EvThread& evthread, const std::string& ip, short port)
{
    std::unique_lock<std::mutex> lock(m_all_opt_mtx);
    if (m_is_running)
        return Errcode{ERR_PREFIX "http server is already running!", emErr::ERR_UNKNOWN};
    m_is_running = true;

    m_http_server = evhttp_new(evthread.GetEventLoop()->GetEventBase()->GetRawBase());

    if (auto err = _BindAddress(ip, port); err.has_value())
        return err;

    // 开启发送事件
    m_send_reply_event = evthread.RegisterEvent(-1, pollevent::EventOpt::PERSIST, [weak_this{weak_from_this()}](int fd, short events, auto eventid) {
        if (auto shared_this = weak_this.lock(); shared_this) {
            shared_this->_ProcessSendReply();
        }
    });

    if (0 != m_send_reply_event->StartListen(50))
        return Errcode{ERR_PREFIX "start listen failed!", emErr::ERR_UNKNOWN};

    return std::nullopt;
}

ErrOpt HttpServer::Route(const std::string& uri, const ReqHandler& handle)
{
    std::lock_guard<std::mutex> guard(m_all_opt_mtx);

    auto req_handle = new OnReqHandle{.server = weak_from_this(), .handle = handle};

    if (auto err = evhttp_set_cb(m_http_server, uri.c_str(), OnRequest, (void*)req_handle); err != 0) {
        delete req_handle;
        return Errcode{ERR_PREFIX "add cb failed! " + std::string((err == -1 ? "repeat" : "unknown")), emErr::ERR_UNKNOWN};
    }

    m_handles[uri] = req_handle;

    return std::nullopt;
}

core::errcode::ErrOpt HttpServer::UnRoute(const std::string& uri)
{
    std::lock_guard<std::mutex> guard(m_all_opt_mtx);
    
    if (0 != evhttp_del_cb(m_http_server, uri.c_str()))
        return Errcode{ERR_PREFIX "del cb failed!", emErr::ERR_UNKNOWN};

    auto it = m_handles.find(uri);
    Assert(it != m_handles.end());    

    delete it->second;
    m_handles.erase(it);

    return std::nullopt;
}

core::errcode::ErrOpt HttpServer::Stop()
{
    std::lock_guard<std::mutex> guard(m_all_opt_mtx);

    if (m_send_reply_event == nullptr)
        return std::nullopt;

    if (0 != m_send_reply_event->CancelListen())
        return Errcode{ERR_PREFIX "stop listen failed!", emErr::ERR_UNKNOWN};

    m_send_reply_event = nullptr;
    
    evhttp_free(m_http_server);
    m_http_server = nullptr;

    m_is_running = false;

    return std::nullopt;
}

void HttpServer::_ProcessSendReply()
{
    std::unique_lock<std::mutex> lock(m_all_opt_mtx);

    while (!m_async_send_response_queue.empty())
    {
        auto resp = m_async_send_response_queue.front();

        if (auto err = resp->_DoSendReply(m_http_server); err != std::nullopt) {
            if (m_onerr) {
                m_onerr(err.value());
            }
        }

        m_async_send_response_queue.pop();
    }
}

core::errcode::ErrOpt HttpServer::AsyncReply(std::shared_ptr<detail::Context> resp)
{
    if (resp == nullptr)
        return Errcode("response is null!", emErr::ERR_UNKNOWN);

    std::lock_guard<std::mutex> guard(m_all_opt_mtx);
    if (!m_is_running)
        return Errcode("http server is not running!", emErr::ERR_UNKNOWN);

    m_async_send_response_queue.push(resp);

    return std::nullopt;
}

void HttpServer::SetErrCallback(const OnErrorCallback& on_err)
{
    std::lock_guard<std::mutex> guard(m_all_opt_mtx);
    m_onerr = on_err;
}


} // namespace bbt::http::ev


#undef ERR_PREFIX