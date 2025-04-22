#pragma once
#include <bbt/http/detail/HttpParser.hpp>

namespace bbt::http::detail
{

class RespParser:
    public HttpParser
{
public:
    RespParser(): HttpParser(HTTP_RESPONSE) {}
    virtual ~RespParser() = default;

    const std::string& GetVersion() const { return m_response_data.url; }
    const std::string& GetStatus() const { return m_response_data.status; }
    const std::string& GetBody() const { return m_response_data.body; }
    const std::unordered_map<std::string, std::string>& GetHeaders() const { return m_response_data.headers; }
    const std::string& GetHeader(const std::string& key, const std::string& default_value) const
    {
        auto it = m_response_data.headers.find(key);
        if (it != m_response_data.headers.end())
            return it->second;
        return default_value;
    }
};

} // namespace bbt::http::detail