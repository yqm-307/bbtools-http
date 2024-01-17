#include <bbt/buffer/Buffer.hpp>
#include <bbt/config/GlobalConfig.hpp>
#include "libevent/HttpServer.hpp"

using namespace bbt::http::ev;


const char Response[] = "{ \
    \"code\"    : 200, \
    \"status\"  : \"Ok\", \
    \"data\"    : {1, 2}\
    }";

int main()
{
    int* debug_log = new int(1);
    BBT_CONFIG_QUICK_SET_DYNAMIC_ENTRY(int, debug_log, bbt::config::_BBTSysCfg::BBT_LOG_STDOUT_OPEN);

    event_base* base = event_base_new();

    HttpServer server{base};

    auto err1 = server.SetHandler("/hello", 
    [](bbt::http::RequestId id, bbt::buffer::Buffer& buf, HttpServer* server)
    {
        auto err = server->DoReply(
            id, 
            200, 
            "ok",
            bbt::buffer::Buffer("{\"code\":200, \"status\":\"OK\", \"data\":\"hello world\""));
        assert(err == std::nullopt);
    });

    if (err1 != std::nullopt) {
        printf("[error] %s\n", err1.value().What().c_str());
    }

    auto err2 = server.SetHandler("/service_110", 
    [](bbt::http::RequestId id, bbt::buffer::Buffer& buf, HttpServer* server)
    {
        auto err = server->DoReply(
            id,
            200,
            "ok",
            bbt::buffer::Buffer("{\"code\":200, \"status\":\"OK\", \"data\":\"call service 110!\"}"));
        assert(err == std::nullopt);
    });

    if (err2 != std::nullopt) {
        printf("[error] %s\n", err2.value().What().c_str());
    }

    auto err_echo = server.SetHandler("/echo", 
    [](bbt::http::RequestId id, bbt::buffer::Buffer& buf, HttpServer* server)
    {
        auto err = server->DoReply(
            id,
            200,
            "ok",
            buf);
        assert(err == std::nullopt);
    });

    if (err_echo != std::nullopt) {
        printf("[error] %s\n", err_echo.value().What().c_str());
    }

    auto err_listen = server.BindListenFd("10.0.2.15", 7777);
    if (err_listen != std::nullopt) {
        printf("[error] %s\n", err_listen.value().What().c_str());
    }

    event_base_dispatch(base);
    event_base_loop(base, EVLOOP_NO_EXIT_ON_EMPTY);
}