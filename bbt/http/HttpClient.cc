#include "HttpClient.hpp"
#include <bbt/http/Request.hpp>

namespace bbt::http
{

size_t OnRecvResponse(void* buf, size_t size, size_t nmemb, void* arg)
{
    auto request = reinterpret_cast<Request*>(arg);
    request->OnRecvResponse((char*)buf, size * nmemb);
    return size * nmemb;
}

HttpClient::HttpClient():
    m_multi_conn(curl_multi_init())
{
}

HttpClient::~HttpClient()
{
    std::lock_guard<std::mutex> lock(m_all_opt_mtx);
    if (m_multi_conn != nullptr) {
        curl_multi_cleanup(m_multi_conn);
        m_multi_conn = nullptr;
    }
}

core::errcode::ErrOpt HttpClient::RunInEvThread(pollevent::EvThread& thread)
{
    m_poll_event = thread.RegisterEvent(-1, EV_PERSIST | EV_TIMEOUT, [this](int fd, short event, pollevent::EventId id) {
        TimeTick();
    });

    if (m_poll_event == nullptr) {
        return core::errcode::Errcode(BBT_HTTP_MODULE_NAME "client regist poll event failed!", emErr::ERR_UNKNOWN);
    }

    if (m_poll_event->StartListen(50) != 0) {
        return core::errcode::Errcode(BBT_HTTP_MODULE_NAME "client start event failed!", emErr::ERR_UNKNOWN);
    }

    return std::nullopt;
}

core::errcode::ErrOpt HttpClient::Stop()
{
    std::lock_guard<std::mutex> lock(m_all_opt_mtx);
    if (m_poll_event == nullptr) {
        return std::nullopt;
    }

    if (0 != m_poll_event->CancelListen()) {
        return core::errcode::Errcode(BBT_HTTP_MODULE_NAME "client stop event failed!", emErr::ERR_UNKNOWN);
    }

    m_poll_event = nullptr;
    return std::nullopt;
}


void HttpClient::TimeTick()
{
    int running_handles = 0;
    int done_handles = 0;
    {
        std::lock_guard<std::mutex> lock(m_all_opt_mtx);
        CURLMcode err = curl_multi_perform(m_multi_conn, &running_handles);
    }

    __CheckDone();
}

core::errcode::ErrOpt HttpClient::ProcessRequestEx(std::shared_ptr<Request> req)
{
    CURL* curl = req->GetCURL();
    int err = 0;

    if (curl == nullptr) {
        return core::errcode::Errcode(BBT_HTTP_MODULE_NAME "client regist poll event failed!", emErr::ERR_UNKNOWN);
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnRecvResponse);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, OnRecvResponse);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, req.get());
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, req.get());
    curl_easy_setopt(curl, CURLOPT_PRIVATE, req.get());

    {
        std::lock_guard<std::mutex> lock(m_all_opt_mtx);
        err = curl_multi_add_handle(m_multi_conn, curl);
    }
    if (err != CURLM_OK) {
        return core::errcode::Errcode(BBT_HTTP_MODULE_NAME "client regist poll event failed!", emErr::ERR_UNKNOWN);
    }

    return std::nullopt;
}

void HttpClient::__CheckDone()
{
    CURLMsg* msg = nullptr;
    Request* req = nullptr;

    std::unique_lock<std::mutex> lock(m_all_opt_mtx);

    do {
        int msgq = 0;
        msg = curl_multi_info_read(m_multi_conn, &msgq);
        if (msg && (msg->msg == CURLMSG_DONE))
        {
            CURL* easy_curl = msg->easy_handle;
            curl_multi_remove_handle(m_multi_conn, easy_curl);
            lock.unlock();

            req = nullptr;
            if (curl_easy_getinfo(easy_curl, CURLINFO_PRIVATE, &req) == CURLcode::CURLE_OK && req != nullptr)
            {

                req->OnComplete(std::nullopt);  // 不知道超时之类的怎么触发的，先都当成功处理
            }
            
            lock.lock();
            curl_multi_remove_handle(m_multi_conn, easy_curl);
        }
    } while(msg);
}

} // namespace bbt::http::ev
