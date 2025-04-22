#pragma once
#include <thread>
#include <map>
#include <atomic>
#include <event2/event.h>
#include <memory>
#include <bbt/core/buffer/Buffer.hpp>
#include <bbt/pollevent/Event.hpp>
#include <bbt/pollevent/EvThread.hpp>
#include <bbt/http/detail/Define.hpp>
#include <bbt/http/detail/ReqParser.hpp>
#include <bbt/http/detail/RespParser.hpp>

namespace bbt::http
{

class HttpClient;

class HttpClient
{
public:
    HttpClient();
    ~HttpClient();

    /**
     * @brief 运行在evthread中，请不要在多线程中使用
     * 
     * @param thread 
     * @return core::errcode::ErrOpt 
     */
    core::errcode::ErrOpt RunInEvThread(pollevent::EvThread& thread);

    /**
     * @brief 停止http client收发
     * 
     * @return core::errcode::ErrOpt 
     */
    core::errcode::ErrOpt Stop();

    /**
     * @brief 执行一个http请求
     * 
     * @param req 
     * @return core::errcode::ErrOpt 
     */
    core::errcode::ErrOpt ProcessRequestEx(std::shared_ptr<Request> req);

private:
    void TimeTick();
    void __CheckDone();

private:
    std::shared_ptr<bbt::pollevent::Event>
                                m_poll_event{nullptr};

    std::mutex                  m_all_opt_mtx;
    CURLM*                      m_multi_conn{NULL};
    std::once_flag              m_once_flag;
};



} // namespace bbt::http::ev
