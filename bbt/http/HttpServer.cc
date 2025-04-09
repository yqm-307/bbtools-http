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

    {
        std::lock_guard<std::mutex> guard(m_all_opt_mtx);

        for (auto&& [uri, onreq_handle] : m_handles)
        {
            delete onreq_handle;
            evhttp_del_cb(m_http_server, uri.c_str());
        }
    }

    evhttp_free(m_http_server);
    
}


ErrOpt HttpServer::_BindAddress(const std::string& ip, short port)
{
    sockaddr_storage ss;
    evutil_socket_t fd;
    ev_socklen_t socklen = sizeof(ss);
    char addr_buf[128];
    void* inaddr;
    const char* addr;
    int got_port = -1;
    evhttp_bound_socket* m_listen_fd = evhttp_bind_socket_with_handle(m_http_server, ip.c_str(), port);

    if (m_listen_fd == nullptr) {
        return Errcode("evhttp_bind_socket_with_handle() failed!", emErr::ERR_UNKNOWN);
    }

    fd = evhttp_bound_socket_get_fd(m_listen_fd);
    memset(&ss, 0, sizeof(ss));

    if (getsockname(fd, (sockaddr*)&ss, &socklen)) {
        return Errcode("getsockname() failed!", emErr::ERR_UNKNOWN);
    }

    got_port = ntohs(((sockaddr_in*)&ss)->sin_port);
    inaddr = &((sockaddr_in*)&ss)->sin_addr;

    addr = evutil_inet_ntop(ss.ss_family, inaddr, addr_buf, sizeof(addr_buf));
    if (addr == NULL) {
        return Errcode("evutil_inet_ntop() failed!", emErr::ERR_UNKNOWN);
    }

    return std::nullopt;
}

ErrOpt HttpServer::RunInEvThread(pollevent::EvThread& evthread, const std::string& ip, short port)
{
    bool expected = false;
    if (m_is_running.compare_exchange_strong(expected, true) == false)
        return std::nullopt;

    m_http_server = evhttp_new(evthread.GetEventLoop()->GetEventBase()->GetRawBase());

    auto err = _BindAddress(ip, port);

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

    auto it = m_handles.find(uri);
    if (it != m_handles.end()) {
        return Errcode("handles repeat!", emErr::ERR_UNKNOWN);
    }

    auto req_handle = new OnReqHandle{.server = weak_from_this(), .handle = handle};

    m_handles[uri] = req_handle;

    if (0 != evhttp_set_cb(m_http_server, uri.c_str(), OnRequest, (void*)m_handles[uri]))
        return Errcode{ERR_PREFIX "add cb failed!", emErr::ERR_UNKNOWN};

    return std::nullopt;
}

void HttpServer::_ProcessRequest(evhttp_request* req)
{
    const evhttp_uri* uri_obj = evhttp_request_get_evhttp_uri(req);
    const char* uri_path = evhttp_uri_get_path(uri_obj);
}

void HttpServer::_ProcessSendReply()
{
    std::lock_guard<std::mutex> guard(m_all_opt_mtx);

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
    if (resp == nullptr) {
        return Errcode("response is null!", emErr::ERR_UNKNOWN);
    }

    std::lock_guard<std::mutex> guard(m_all_opt_mtx);
    m_async_send_response_queue.push(resp);

    return std::nullopt;
}

void HttpServer::SetErrCallback(const OnErrorCallback& on_err)
{
    m_onerr = on_err;
}


} // namespace bbt::http::ev


#undef ERR_PREFIX