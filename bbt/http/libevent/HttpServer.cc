#include <cstring>

#include <event2/keyvalq_struct.h>
#include <bbt/base/Logger/Logger.hpp>
#include "./HttpServer.hpp"


namespace bbt::http::ev
{

void EventHttpRequest(evhttp_request* req, void* arg)
{
    evbuffer* evbuf;

    auto pthis = reinterpret_cast<HttpServer*>(arg);
    
    int code = 0;
    if (pthis == nullptr) {
        return;
    }

    auto err = pthis->__Handler(req);
    if (err != std::nullopt) {
        BBT_FULL_LOG_ERROR(err.value().What().c_str());
        return;
    }
}

bbt::buffer::Buffer Req2Buffer(evhttp_request* req)
{
    buffer::Buffer ybuf;
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

bbt::buffer::Buffer Req2Header(evhttp_request* req)
{
    buffer::Buffer ybuf;
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

HttpServer::HttpServer(event_base* ev)
    :m_io_ctx(ev)
{
    m_http_server = evhttp_new(m_io_ctx);

}

HttpServer::~HttpServer()
{
    for (auto&& uri : m_handles)
    {
        __DelHandler(uri.first);
    }

    evhttp_free(m_http_server);
    
}


ErrOpt HttpServer::__BindAddress(const std::string& ip, short port)
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
        return Errcode("evhttp_bind_socket_with_handle() failed!");
    }

    fd = evhttp_bound_socket_get_fd(m_listen_fd);
    memset(&ss, 0, sizeof(ss));

    if (getsockname(fd, (sockaddr*)&ss, &socklen)) {
        return Errcode("getsockname() failed!");
    }

    got_port = ntohs(((sockaddr_in*)&ss)->sin_port);
    inaddr = &((sockaddr_in*)&ss)->sin_addr;

    addr = evutil_inet_ntop(ss.ss_family, inaddr, addr_buf, sizeof(addr_buf));
    if (addr == NULL) {
        return Errcode("evutil_inet_ntop() failed!");
    }

    return std::nullopt;
}

ErrOpt HttpServer::BindListenFd(const std::string& ip, short port)
{
    if (m_is_running) {
        return std::nullopt;
    }

    auto err = __BindAddress(ip, port);
    if (err != std::nullopt) {
        return err;
    }

    m_is_running = true;
    return std::nullopt;
}


ErrOpt HttpServer::SetHandler(const std::string& path, ReqHandler cb)
{
    auto it = m_handles.find(path);
    if (it != m_handles.end()) {
        return Errcode("handles repeat!", emErr::Failed);
    }

    m_handles.insert(std::make_pair(path, cb));

    return __AddHandler(path);
}

ErrOpt HttpServer::__Handler(evhttp_request* req)
{
    auto uri = evhttp_request_get_evhttp_uri(req);
    std::string uri_path = evhttp_uri_get_path(uri);

    auto it = m_handles.find(uri_path);
    if (it != m_handles.end()) {
        auto [err, id] = __OnRequest(req);
        if (err != std::nullopt) {
            return err;
        }

        bbt::buffer::Buffer header(Req2Header(req));
        bbt::buffer::Buffer buf(Req2Buffer(req));
        BBT_FULL_LOG_INFO("[EventHttpRequest] %s", buf.Peek());
        it->second(id, buf, this);
    } else {
        return Errcode(uri_path + " is not exist service(uri)");
    }
    //TODO 走默认处理

    return std::nullopt;
}

ErrOpt HttpServer::DoReply(RequestId id, int code, std::string status, const buffer::Buffer& buf)
{
    auto it = m_wait_requests.find(id);
    assert(it != m_wait_requests.end());

    evbuffer* evbuf = evbuffer_new();
    if (evbuffer_add(evbuf, buf.Peek(), buf.DataSize()) != 0) {
        return Errcode("evbuffer_add() failed!");
    }

    evhttp_send_reply(it->second, code, status.c_str(), evbuf);
    evbuffer_free(evbuf);
    m_wait_requests.erase(id);

    std::string str{buf.Peek(), buf.DataSize()};
    BBT_FULL_LOG_INFO("send success: %s", str.c_str());
    return std::nullopt;
}

std::pair<ErrOpt, RequestId> HttpServer::__OnRequest(evhttp_request* req)
{
    RequestId id = GenerateRequestId();

    auto [_, isok] = m_wait_requests.insert(std::make_pair(id, req));
    assert(isok);

    return {std::nullopt, id};
}

ErrOpt HttpServer::__AddHandler(const std::string& uri)
{
    int err = evhttp_set_cb(m_http_server, uri.c_str(), EventHttpRequest, this);
    if (err == -1) {
        return Errcode("cb already exist!");
    } else if (err == -2){
        return Errcode("evhttp_set_cb() failed!");
    }

    return std::nullopt;
}

ErrOpt HttpServer::__DelHandler(const std::string& uri)
{
    int err = evhttp_del_cb(m_http_server, uri.c_str());
    if (err != 0) {
        return Errcode("evhttp_del_cb() failed!");
    }

    return std::nullopt;
}


} // namespace bbt::http::ev
