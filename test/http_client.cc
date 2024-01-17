#include <bbt/buffer/Buffer.hpp>
#include <bbt/config/GlobalConfig.hpp>
#include "libevent/HttpClient.hpp"

using namespace bbt::http::ev;

void TimeOut(evutil_socket_t fd, short event, void* arg)
{
    bbt::buffer::Buffer buf{"{\"key\":\"value\"}"};
    auto client = reinterpret_cast<HttpClient*>(arg);
    auto err = client->PostReq("http://10.0.2.15:7777/echo", buf, 
    [](CURL* _, bbt::buffer::Buffer& buf, CURLcode code)
    {
        assert(code == CURLE_OK);
        std::string str{buf.Peek(), buf.DataSize()};
        printf("echo reply: %s\n", str.c_str());
    });
    
    if (err != std::nullopt) {
        printf("[error] %s\n", err.value().What().c_str());
    }
}

int main()
{
    event_base* base = event_base_new();
    HttpClient client{base};

    event* timer = event_new(base, -1, EV_PERSIST | EV_TIMEOUT, TimeOut, (void*)&client);

    timeval tm;
    evutil_timerclear(&tm);
    tm.tv_sec = 1;
    event_add(timer, &tm);

    event_base_loop(base, EVLOOP_NO_EXIT_ON_EMPTY);
    event_base_free(base);
}