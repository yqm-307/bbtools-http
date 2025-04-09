#pragma once
#include <thread>
#include <map>
#include <atomic>
#include <event2/event.h>
#include <memory>
#include <bbt/core/buffer/Buffer.hpp>
#include <bbt/http/detail/Define.hpp>
#include <bbt/pollevent/Event.hpp>
#include <bbt/pollevent/EvThread.hpp>

namespace bbt::http
{

class HttpClient;

class HttpClient
{
public:
    HttpClient();
    ~HttpClient();

    core::errcode::ErrOpt RunInEvThread(pollevent::EvThread& thread);
    /**
     * @brief 执行一个http请求
     * 
     * @param req 
     * @return core::errcode::ErrOpt 
     */
    core::errcode::ErrOpt ProcessRequestEx(Request* req);

    void RunOnce();
    void TimeTick();
protected:
    void __CheckDone();

private:
    std::shared_ptr<bbt::pollevent::Event> m_poll_event{nullptr};
    CURLM*                      m_multi_conn{NULL};
    static std::atomic_int64_t  s_request_id;
};



} // namespace bbt::http::ev
