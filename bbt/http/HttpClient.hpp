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


// 请求携带的私有数据
struct RequestPrivData
{
    HttpClient* m_pthis{NULL};
    uint64_t    m_request_id{0};
};

struct RequestData
{
    RespHandler m_response_callback;
    CURL*       m_curl;
    curl_slist* m_req_headers;
    RequestId   m_id;
    core::Buffer m_response_buffer;
    RequestPrivData m_write_priv_data;
};

class HttpClient
{
    friend size_t CurlWrite(void* buf, size_t size, size_t nmemb, void* arg);
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
