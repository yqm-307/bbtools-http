#include "HttpClient.hpp"

namespace bbt::http::ev
{

//FIXME 这里理解错误，write handle不是请求完成，而是socket接收到数据
// 后续需要添加一个map保存信息
size_t CurlWrite(void* buf, size_t size, size_t nmemb, void* arg)
{
    auto private_data = reinterpret_cast<RequestPrivData*>(arg);
    auto pthis = private_data->m_pthis;
    auto data_ptr = pthis->m_request_wait_map[private_data->m_request_id];

    buffer::Buffer body((char*)buf, (size * nmemb));
    data_ptr->m_response_callback(data_ptr->m_curl, body);

    //FIXME 这里释放对于libcurl来说是否合法？
    curl_easy_cleanup(data_ptr->m_curl);
    curl_slist_free_all(data_ptr->m_list);
    size_t pos = pthis->m_request_wait_map.erase(data_ptr->m_id);
    assert(pos == 1);
    delete private_data;

    return size * nmemb;
}

void EventTimeOut(evutil_socket_t, short event, void* prvdata)
{
    auto pthis = reinterpret_cast<HttpClient*>(prvdata);

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
    curl_multi_cleanup(m_multi_conn);
    // curl_multi_info_read();
}

void HttpClient::TimeTick()
{
    __OnTime50Ms();
}

ErrOpt HttpClient::PostReq(const char* url, buffer::Buffer& body, RespHandler cb)
{
    auto ptr = std::make_shared<RequestData>();
    RequestId id = GenerateRequestId();
    assert(id >= 0);
    ptr->m_id = id;
    auto priv_data = new RequestPrivData;
    priv_data->m_request_id = id;
    priv_data->m_pthis = this;

    CURL* easy_curl = curl_easy_init();
    curl_slist* list = NULL;
    
    /* 设置http协议头 */
    list = curl_slist_append(list, "Content-Type: application/json");
    curl_easy_setopt(easy_curl, CURLOPT_HTTPHEADER, list);

    curl_easy_setopt(easy_curl, CURLOPT_URL, url);
    curl_easy_setopt(easy_curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(easy_curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(easy_curl, CURLOPT_TIMEOUT_MS, UPDATE_TIME_MS);
    curl_easy_setopt(easy_curl, CURLOPT_WRITEFUNCTION, CurlWrite);
    curl_easy_setopt(easy_curl, CURLOPT_WRITEDATA, priv_data);

    /* 设置 post 数据 */
    curl_easy_setopt(easy_curl, CURLOPT_POST, 1L);
    curl_easy_setopt(easy_curl, CURLOPT_POSTFIELDSIZE, body.DataSize());
    curl_easy_setopt(easy_curl, CURLOPT_COPYPOSTFIELDS, body.Peek());

    int err = curl_multi_add_handle(m_multi_conn, easy_curl);

    if (err != CURLM_OK) {
        curl_easy_cleanup(easy_curl);
        curl_slist_free_all(list);
        return Errcode("curl_multi_add_handle() failed! CURLMcode" + std::to_string(err));
    }

    ptr->m_curl = easy_curl;
    ptr->m_list = list;
    ptr->m_response_callback = cb;
    auto [_, isok] = m_request_wait_map.insert(std::make_pair(id, ptr));
    assert(isok);

    return std::nullopt;
}


void HttpClient::__RegistTimerEvent()
{
    m_timer = event_new(m_io_ctx, -1, EV_PERSIST | EV_TIMEOUT, EventTimeOut, this);

    timeval tm;
    evutil_timerclear(&tm);
    tm.tv_usec = UPDATE_TIME_MS * 1000;

    event_add(m_timer, &tm);
}

void HttpClient::__UnRegistTimerEvent()
{
    event_del(m_timer);
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
