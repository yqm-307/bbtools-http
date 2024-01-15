#include "libevent/HttpServer.hpp"

using namespace bbt::http::ev;

#define Response "hello world"

int main()
{

    event_base* base = event_base_new();

    HttpServer server{base};

    server.SetHandler("/hello", [](evhttp_request* req, HttpServer* server){
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

    server.Listen("", 7777);

    event_base_dispatch(base);
}