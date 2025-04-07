#include "HttpClient.hpp"

namespace bbt::http::ev
{

std::atomic_int64_t HttpClient::s_request_id{0};

//FIXME 这里理解错误，write handle不是请求完成，而是socket接收到数据
// 后续需要添加一个map保存信息
size_t CurlWrite(void* buf, size_t size, size_t nmemb, void* arg)
{
    auto private_data = reinterpret_cast<RequestPrivData*>(arg);
    auto pthis = private_data->m_pthis;
    auto data_ptr = pthis->m_request_wait_map[private_data->m_request_id];

    /* 缓存起来 */
    assert(data_ptr->m_response_buffer.WriteString((char*)buf, (size * nmemb)));
    std::string str{data_ptr->m_response_buffer.Peek(), data_ptr->m_response_buffer.Size()};

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

core::errcode::ErrOpt HttpClient::PostReq(const char* url, core::Buffer& body, RespHandler cb)
{
    auto ptr = std::make_shared<RequestData>();
    RequestId id = ++s_request_id;
    assert(id >= 0);
    ptr->m_id = id;
    // auto priv_data = new RequestPrivData;
    ptr->m_write_priv_data.m_request_id = id;
    ptr->m_write_priv_data.m_pthis = this;

    CURL* easy_curl = curl_easy_init();
    curl_slist* list = NULL;
    
    /* 设置http协议头 */
    list = curl_slist_append(list, "Content-Type: application/json");
    curl_easy_setopt(easy_curl, CURLOPT_HTTPHEADER, list);

    curl_easy_setopt(easy_curl, CURLOPT_URL, url);
    curl_easy_setopt(easy_curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(easy_curl, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(easy_curl, CURLOPT_TIMEOUT_MS, HTTP_SERVER_REQUEST_TIMEOUT_MS);
    curl_easy_setopt(easy_curl, CURLOPT_WRITEFUNCTION, CurlWrite);
    curl_easy_setopt(easy_curl, CURLOPT_WRITEDATA, &(ptr->m_write_priv_data));

    /* 设置 post 数据 */
    curl_easy_setopt(easy_curl, CURLOPT_POST, 1L);
    curl_easy_setopt(easy_curl, CURLOPT_POSTFIELDSIZE, body.Size());
    curl_easy_setopt(easy_curl, CURLOPT_COPYPOSTFIELDS, body.Peek());

    int err = curl_multi_add_handle(m_multi_conn, easy_curl);

    if (err != CURLM_OK) {
        curl_easy_cleanup(easy_curl);
        curl_slist_free_all(list);
        return core::errcode::Errcode("curl_multi_add_handle() failed! CURLMcode" + std::to_string(err), emErr::ERR_UNKNOWN);
    }

    ptr->m_curl = easy_curl;
    ptr->m_req_headers = list;
    ptr->m_response_callback = cb;
    __RegistCURL(id, ptr);
    // auto [_, isok] = m_request_wait_map.insert(std::make_pair(id, ptr));
    // assert(isok);

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
    int done_handles = 0;
    CURLMcode err = curl_multi_perform(m_multi_conn, &running_handles);

    // err = curl_multi_poll(m_multi_conn, NULL, 0, 1, &done_handles);
    // if (err == CURLM_OK && done_handles > 0) {
    __CheckDone();
    // }
}

void HttpClient::__OnTime50Ms()
{
    /* 函数每 50ms 执行一次，不保证精准，以下是需要执行的操作 */
    RunOnce();
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
            auto it = m_curl_map.find(easy_curl);
            if (it == m_curl_map.end()) {
                curl_multi_remove_handle(m_multi_conn, easy_curl);
                continue;
            }
            
            __HandleResponse(it->second, msg->data.result);
            curl_multi_remove_handle(m_multi_conn, easy_curl);
        }
    } while(msg);
}

void HttpClient::__RegistCURL(RequestId id, std::shared_ptr<RequestData> data_ptr)
{
    auto [_1, isok1] = m_curl_map.insert(std::make_pair(data_ptr->m_curl, id));
    assert(isok1);
    auto [_2, isok2] = m_request_wait_map.insert(std::make_pair(data_ptr->m_id, data_ptr));
    assert(isok2);
}

void HttpClient::__UnRegistCURL(RequestId id)
{
    auto data_it = m_request_wait_map.find(id);
    assert(data_it != m_request_wait_map.end());

    CURL* curl = data_it->second->m_curl;

    assert(m_curl_map.erase(curl)           == 1);
    assert(m_request_wait_map.erase(id)     == 1);

    curl_slist_free_all(data_it->second->m_req_headers);
    curl_easy_cleanup(data_it->second->m_curl);
}

void HttpClient::__HandleResponse(RequestId id, CURLcode code)
{
    auto it = m_request_wait_map.find(id);
    assert(it != m_request_wait_map.end());

    auto data_ptr = it->second;
    data_ptr->m_response_callback(data_ptr->m_curl, data_ptr->m_response_buffer, code);

    __UnRegistCURL(id);
}



} // namespace bbt::http::ev
