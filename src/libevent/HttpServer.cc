#include "./HttpServer.hpp"

#include <cstring>

namespace bbt::http::ev
{

void EventHttpRequest(evhttp_request* req, void* arg)
{
    evbuffer* evbuf;
    static char buf[HTTP_SERVER_PROTOCOL_MAX_LENGTH];
    size_t length = 0;

    int n = 0;
    evbuf = evhttp_request_get_input_buffer(req);
    while (evbuffer_get_length(evbuf) > 0) {
        n = evbuffer_remove(evbuf, buf, sizeof(buf));
        length += n;
    }

    auto* prvdata = static_cast<EventHttpRequestPrvData*>(arg);

    auto pthis = prvdata->m_wptr.lock();
    
    std::string resp = "";
    int code = 0;
    if (pthis) {
        pthis->__Handler(req);
    }
}

HttpServer::HttpServer(event_base* ev)
    :m_io_ctx(ev)
{
}

HttpServer::~HttpServer()
{

}


ErrOpt HttpServer::__Listen()
{
    evhttp* httpd = NULL;
}

ErrOpt HttpServer::Listen(const std::string& ip, short port)
{
    sockaddr_storage ss;
    evutil_socket_t fd;
    ev_socklen_t socklen = sizeof(ss);
    char addr_buf[128];
    void* inaddr;
    const char* addr;
    int got_port = -1;

    m_http_server = evhttp_new(m_io_ctx);
    m_http_socket = evhttp_bind_socket_with_handle(m_http_server, ip.c_str(), port);

    if (!m_http_socket) {
        return Errcode("evhttp_bind_socket_with_handle() failed!");
    }

    fd = evhttp_bound_socket_get_fd(m_http_socket);
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


ErrOpt HttpServer::SetHandler(const std::string& path, ReqHandler cb)
{
    auto it = m_handles.find(path);
    if (it != m_handles.end()) {
        return Errcode("handles repeat!", emErr::Failed);
    }

    m_handles.insert(std::make_pair(path, cb));

    return std::nullopt;
}

void HttpServer::__Handler(evhttp_request* req)
{
    auto uri = evhttp_request_get_evhttp_uri(req);
    std::string uri_path = evhttp_uri_get_path(uri);
    printf("%s\n", uri_path.c_str());
    auto it = m_handles.find(uri_path);
    if (it != m_handles.end()) {
        it->second(req, this);
    }
    //TODO 走默认处理
}

ErrOpt HttpServer::__AddHandler(const std::string& uri)
{
    int err = evhttp_set_cb(m_http_server, uri.c_str(), EventHttpRequest, NULL);
    if (err == -1) {
        return Errcode("cb already exist!");
    } else if (err == -2){
        return Errcode("evhttp_set_cb() failed!");
    }
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
