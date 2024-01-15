#pragma once
#include <thread>
#include <atomic>
#include <event2/event.h>
#include <memory>
#include "engine/IHttpClient.hpp"

namespace bbt::http::ev
{

class HttpClient;

struct EventTimeOutPrvData
{
    std::weak_ptr<HttpClient> m_wptr;
};

#define UPDATE_TIME_MS 50


class HttpClient:
    public IHttpClient,
    public std::enable_shared_from_this<HttpClient>
{
public:
    HttpClient(event_base* io_ctx);
    ~HttpClient();

    virtual ErrOpt Request(const char* url, const char* body, HttpHandler do_resp_handler) override;
    virtual ErrOpt Request(const char* ip, short port, const char* body, HttpHandler do_resp_handler) override;

    void RunOnce();
    void TimeTick();
protected:
    /* 50ms 触发一次（有误差） */
    void __OnTime50Ms();
    void __RegistTimerEvent();
    void __UnRegistTimerEvent();
private:
    event_base*                 m_io_ctx{NULL};
    event*                      m_timer{NULL};
    EventTimeOutPrvData*        m_timer_prvdata{NULL};
    CURLM*                      m_multi_conn{NULL};
    volatile bool               m_running{true};

    std::atomic_int             m_running_handles{0};
};



} // namespace bbt::http::ev
