#include <cstdarg>
#include <memory>
#include <bbt/http/Request.hpp>
#include <bbt/http/detail/RespParser.hpp>

#define ERR_PREFIX BBT_HTTP_MODULE_NAME "[Request]"

using namespace bbt::core::errcode;

namespace bbt::http
{

Request::Request():
    m_curl(curl_easy_init())
{
}

Request::~Request()
{
    // llhttp_free(m_response_parser);
    curl_easy_cleanup(m_curl);
    m_curl = nullptr;
}

bool Request::IsCompleted()
{
    return m_is_completed;
}

void Request::Clear()
{

}


ErrOpt Request::SetOpt(emHttpOpt opt, ...)
{
    if (m_is_completed)
        return Errcode{ERR_PREFIX "[Request] already completed!", emErr::ERR_UNKNOWN};

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

core::errcode::ErrTuple<std::shared_ptr<detail::RespParser>> Request::Parse() const
{
    auto resp_parser = std::make_shared<detail::RespParser>();
    if (auto err = resp_parser->ExecuteParse(m_response_data.Peek(), m_response_data.Size()); err != std::nullopt) {
        return {Errcode{ERR_PREFIX "Parse response failed!", emErr::ERR_UNKNOWN}, nullptr};
    }

    return {std::nullopt, resp_parser};
}

void Request::OnRecvResponse(const char* header, size_t len)
{
    m_response_data.WriteString(header, len);
}

void Request::OnComplete(core::errcode::ErrOpt err)
{
    m_is_completed = true;
    if (m_on_resp_cb)
        m_on_resp_cb(err);
}

CURL* Request::GetCURL()
{
    return m_curl;
}

const bbt::core::Buffer& Request::GetRawResponse() const
{
    return m_response_data;
}

} // namespace bbt::http


#undef ERR_PREFIX