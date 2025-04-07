#include <cstring>

#include <event2/keyvalq_struct.h>
#include <bbt/http/HttpServer.hpp>


using namespace bbt::core;
using namespace bbt::core::errcode;

namespace bbt::http
{

std::atomic_uint64_t HttpServer::s_request_id{0};

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
        return;
    }
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
        return Errcode("handles repeat!", emErr::ERR_UNKNOWN);
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

        Buffer header(Req2Header(req));
        Buffer buf(Req2Buffer(req));
        it->second(id, buf, this);
    } else {
        return Errcode(uri_path + " is not exist service(uri)", emErr::ERR_UNKNOWN);
    }
    //TODO 走默认处理

    return std::nullopt;
}

ErrOpt HttpServer::DoReply(RequestId id, int code, std::string status, const Buffer& buf)
{
    auto it = m_wait_requests.find(id);
    assert(it != m_wait_requests.end());

    evbuffer* evbuf = evbuffer_new();
    if (evbuffer_add(evbuf, buf.Peek(), buf.Size()) != 0) {
        return Errcode("evbuffer_add() failed!", emErr::ERR_UNKNOWN);
    }

    evhttp_send_reply(it->second, code, status.c_str(), evbuf);
    evbuffer_free(evbuf);
    m_wait_requests.erase(id);

    std::string str{buf.Peek(), buf.Size()};
    return std::nullopt;
}

ErrTuple<RequestId> HttpServer::__OnRequest(evhttp_request* req)
{
    RequestId id = ++s_request_id;

    auto [_, isok] = m_wait_requests.insert(std::make_pair(id, req));
    assert(isok);

    return {std::nullopt, id};
}

ErrOpt HttpServer::__AddHandler(const std::string& uri)
{
    int err = evhttp_set_cb(m_http_server, uri.c_str(), EventHttpRequest, this);
    if (err == -1) {
        return Errcode("cb already exist!", emErr::ERR_UNKNOWN);
    } else if (err == -2){
        return Errcode("evhttp_set_cb() failed!", emErr::ERR_UNKNOWN);
    }

    return std::nullopt;
}

ErrOpt HttpServer::__DelHandler(const std::string& uri)
{
    int err = evhttp_del_cb(m_http_server, uri.c_str());
    if (err != 0) {
        return Errcode("evhttp_del_cb() failed!", emErr::ERR_UNKNOWN);
    }

    return std::nullopt;
}


} // namespace bbt::http::ev
