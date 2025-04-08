#include "HttpClient.hpp"
#include <bbt/http/Request.hpp>

namespace bbt::http
{

std::atomic_int64_t HttpClient::s_request_id{0};

size_t OnRecvHeader(void* buf, size_t size, size_t nmemb, void* arg)
{
    auto request = reinterpret_cast<Request*>(arg);
    request->OnRecvHeader((char*)buf, size * nmemb);
    return size * nmemb;
}

size_t OnRecvBody(void* buf, size_t size, size_t nmemb, void* arg)
{
    auto request = reinterpret_cast<Request*>(arg);
    request->OnRecvBody((char*)buf, size * nmemb);

    return size * nmemb;
}

HttpClient::HttpClient()
{
    m_multi_conn = curl_multi_init();
}

HttpClient::~HttpClient()
{
    curl_multi_cleanup(m_multi_conn);
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


void HttpClient::TimeTick()
{
    RunOnce();
}

core::errcode::ErrOpt HttpClient::ProcessRequestEx(Request* req)
{
    CURL* curl = req->GetCURL();
    int err = 0;

    if (curl == nullptr) {
        return core::errcode::Errcode(BBT_HTTP_MODULE_NAME "client regist poll event failed!", emErr::ERR_UNKNOWN);
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, OnRecvBody);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, OnRecvHeader);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, req);
    curl_easy_setopt(curl, CURLOPT_HEADERDATA, req);
    curl_easy_setopt(curl, CURLOPT_PRIVATE, req);

    err = curl_multi_add_handle(m_multi_conn, curl);
    if (err != CURLM_OK) {
        return core::errcode::Errcode(BBT_HTTP_MODULE_NAME "client regist poll event failed!", emErr::ERR_UNKNOWN);
    }

    return std::nullopt;
}

void HttpClient::RunOnce()
{
    int running_handles = 0;
    int done_handles = 0;
    CURLMcode err = curl_multi_perform(m_multi_conn, &running_handles);

    __CheckDone();
}

void HttpClient::__CheckDone()
{
    CURLMsg* msg = nullptr;
    do {
        int msgq = 0;
        msg = curl_multi_info_read(m_multi_conn, &msgq);
        if (msg && (msg->msg == CURLMSG_DONE))
        {
            CURL* easy_curl = msg->easy_handle;
            curl_multi_remove_handle(m_multi_conn, easy_curl);
            Request* req = nullptr;
            if (curl_easy_getinfo(easy_curl, CURLINFO_PRIVATE, &req) == CURLcode::CURLE_OK && req != nullptr)
            {
                req->OnComplete(std::nullopt);  // 不知道超时之类的怎么触发的，先都当成功处理
            }

            curl_multi_remove_handle(m_multi_conn, easy_curl);
        }
    } while(msg);
}

} // namespace bbt::http::ev
