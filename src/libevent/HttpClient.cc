#include "HttpClient.hpp"


namespace bbt::http::ev
{

size_t CurlWrite(void* buf, size_t size, size_t nmemb, void* stream)
{
    printf("%s\n", (char*)buf);
    printf("%ld\n", size * nmemb);
    return size * nmemb;
}

CURL* CreateEasyCurl(const char* url, const char* req, int timeout_ms)
{
    CURL* curl = curl_easy_init();

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);

    if (timeout_ms > 0) {
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, timeout_ms);
    }

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, CurlWrite);
}

void EventTimeOut(evutil_socket_t, short event, void* prvdata)
{
    EventTimeOutPrvData* data = reinterpret_cast<EventTimeOutPrvData*>(prvdata);

    auto pthis = data->m_wptr.lock();
    if (pthis == nullptr) {
        return;
    }

    pthis->TimeTick();
}

HttpClient::HttpClient(event_base* io_ctx)
    :m_io_ctx(io_ctx)
{
    m_multi_conn = curl_multi_init();
    __RegistTimerEvent();
}

HttpClient::~HttpClient()
{
    __UnRegistTimerEvent();
}

void HttpClient::TimeTick()
{
    __OnTime50Ms();
}

ErrOpt HttpClient::Request(const char* url, const char* request, HttpHandler do_resp_handler)
{
    CURL* easy_curl = CreateEasyCurl(url, request, 1000);
    CURLMcode err = curl_multi_add_handle(m_multi_conn, easy_curl);
    if (err != CURLM_OK) {
        return Errcode("curl_multi_add_handle failed! CURLMcode=" + std::to_string(err), emErr::Failed);
    }

    return std::nullopt;
}

ErrOpt HttpClient::Request(const char* ip, short port, const char* body, HttpHandler do_resp_handler)
{

}

void HttpClient::__RegistTimerEvent()
{
    m_timer_prvdata = new EventTimeOutPrvData();
    m_timer_prvdata->m_wptr = weak_from_this();
    m_timer = event_new(m_io_ctx, -1, EV_PERSIST | EV_TIMEOUT, EventTimeOut, m_timer_prvdata);

    timeval tm;
    evutil_timerclear(&tm);
    tm.tv_sec = UPDATE_TIME_MS * 1000;

    event_add(m_timer, &tm);
}

void HttpClient::__UnRegistTimerEvent()
{
    event_del(m_timer);
    delete m_timer_prvdata;
}

void HttpClient::RunOnce()
{
    int running_handles = 0;
    CURLMcode err = curl_multi_perform(m_multi_conn, &running_handles);
}

void HttpClient::__OnTime50Ms()
{
    /* 函数每 50ms 执行一次，不保证精准，以下是需要执行的操作 */
    RunOnce();
}

} // namespace bbt::http::ev
