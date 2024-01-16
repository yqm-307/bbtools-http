#include "libevent/HttpServer.hpp"

using namespace bbt::http::ev;

#define Response "hello world"

int main()
{

    event_base* base = event_base_new();

    HttpServer server{base};

    auto err1 = server.SetHandler("/hello", [](evhttp_request* req, HttpServer* server){
        evbuffer* req_buf = evhttp_request_get_input_buffer(req);

        while (evbuffer_get_length(req_buf) > 0)
        {
            int n;
            char cbuf[1024];
            n = evbuffer_remove(req_buf, cbuf, sizeof(cbuf));
            if (n > 0)
                fwrite(cbuf, 1, n, stdout);
        }
        
        evbuffer* buf = evbuffer_new();
        evbuffer_add(buf, Response, sizeof(Response));
        evhttp_send_reply(req, 200, "OK", buf);
    });

    if (err1 != std::nullopt) {
        printf("[error] %s\n", err1.value().What().c_str());
    }

    auto err2 = server.Listen("127.0.0.1", 7777);
    if (err2 != std::nullopt) {
        printf("[error] %s\n", err2.value().What().c_str());
    }

    event_base_dispatch(base);
    event_base_loop(base, EVLOOP_NO_EXIT_ON_EMPTY);
}