#pragma once
#include <bbt/http/detail/HttpParser.hpp>

namespace bbt::http::detail
{

class ReqParser:
    public HttpParser
{
public:
    ReqParser(): HttpParser(HTTP_REQUEST) {}
    virtual ~ReqParser() = default;

    emHttpMethod        GetMethod() const { return m_response_data.method; }
    const std::string&  GetUrl() const { return m_response_data.url; }
    const std::string&  GetVersion() const { return m_response_data.version; }
    const std::string&  GetBody() const { return m_response_data.body; }
    const std::unordered_map<std::string, std::string>& GetHeaders() const { return m_response_data.headers; }
    const std::string&  GetHeader(const std::string& key, const std::string& default_value) const
    {
        auto it = m_response_data.headers.find(key);
        if (it != m_response_data.headers.end())
            return it->second;
        return default_value;
    }

};

} // namespace bbt::http::detail