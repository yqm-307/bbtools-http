#include <cstdarg>
#include <bbt/http/Request.hpp>

#define ERR_PREFIX BBT_HTTP_MODULE_NAME "[Request]"

using namespace bbt::core::errcode;

namespace bbt::http
{


size_t WriteHeaderCallback(void* contents, size_t size, size_t nmemb, void* userp);
size_t WriteBodyCallback(void* contents, size_t size, size_t nmemb, void* userp);


Request::Request():
    m_curl(curl_easy_init())
{
}

Request::~Request()
{
    curl_easy_cleanup(m_curl);
    m_curl = nullptr;
}

bool Request::IsCompleted()
{
    return m_is_completed.load();
}

ErrOpt Request::SetOpt(emHttpOpt opt, ...)
{
    if (!m_curl)
        return Errcode{ERR_PREFIX "[Request] bad curl!", emErr::ERR_UNKNOWN};

    va_list args;
    va_start(args, opt);

    CURLcode res = curl_easy_setopt(m_curl, opt, va_arg(args, void*));

    va_end(args);

    if (res != CURLE_OK)
        return Errcode{ERR_PREFIX "[Request] CURLcode=" + std::to_string(res) + " " + std::string(curl_easy_strerror(res)) , emErr::ERR_UNKNOWN};
    
    return std::nullopt;
}

void Request::SetResponseCallback(const ResponseCallback& cb)
{
    m_on_resp_cb = cb;
}

void Request::OnRecvHeader(const char* header, size_t len)
{
    m_response_header.WriteString(header, len);
}

void Request::OnRecvBody(const char* body, size_t len)
{
    m_response_header.WriteString(body, len);
}

void Request::OnComplete(core::errcode::ErrOpt err)
{
    m_is_completed.store(true);
    if (m_on_resp_cb)
        m_on_resp_cb(err, this);
}

CURL* Request::GetCURL()
{
    return m_curl;
}

const bbt::core::Buffer& Request::GetHeader() const
{
    return m_response_header;
}

const bbt::core::Buffer& Request::GetBody() const
{
    return m_response_body;
}

} // namespace bbt::http


#undef ERR_PREFIX