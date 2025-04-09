#include <bbt/http/detail/HttpParser.hpp>

#define ERR_PREFIX BBT_HTTP_MODULE_NAME "[HttpParser]"

using namespace bbt::core::errcode;

namespace bbt::http::detail
{

HttpParser::HttpParser(llhttp_type type):
    m_response_parser(new llhttp_t())
{
    llhttp_init(m_response_parser, type, nullptr);
}

HttpParser::~HttpParser()
{
    delete m_response_parser;
    m_response_parser = nullptr;
}

core::errcode::ErrOpt HttpParser::ExecuteParse(const char* data, size_t len)
{
    if (auto err = _PreExecuteParse(); err != std::nullopt) {
        return err;
    }

    if (auto err = _DoExecuteParse(data, len); err != std::nullopt) {
        return err;
    }

    if (auto err = _FinishExecuteParse(); err != std::nullopt) {
        return err;
    }

    return std::nullopt;
}

core::errcode::ErrOpt HttpParser::_PreExecuteParse()
{
    llhttp_settings_init(&m_settings);
    m_settings.on_url = OnParseUrl;
    m_settings.on_status = OnParseStatus;
    m_settings.on_header_field = OnParseHeaderField;
    m_settings.on_header_value = OnParseHeaderValue;
    m_settings.on_headers_complete = OnParseHeadersComplete;
    m_settings.on_body = OnParseBody;
    m_settings.on_message_complete = OnParseMessageComplete;

    m_response_parser->settings = (void*)&m_settings;
    m_response_parser->data = this;

    return std::nullopt;
}

core::errcode::ErrOpt HttpParser::_DoExecuteParse(const char* data, size_t len)
{
    if (HPE_OK != llhttp_execute(m_response_parser, data, len)) {
        return Errcode{ERR_PREFIX "ExecuteParse llhttp_err=" + std::to_string(llhttp_get_errno(m_response_parser)) + " " + std::string(llhttp_get_error_reason(m_response_parser)), emErr::ERR_UNKNOWN};
    }

    return std::nullopt;
}

core::errcode::ErrOpt HttpParser::_FinishExecuteParse()
{
    if (HPE_OK != llhttp_finish(m_response_parser)) {
        return Errcode{ERR_PREFIX "ExecuteParse llhttp_err=" + std::to_string(llhttp_get_errno(m_response_parser)) + " " + std::string(llhttp_get_error_reason(m_response_parser)), emErr::ERR_UNKNOWN};
    }

    return std::nullopt;
}


int HttpParser::OnParseUrl(llhttp_t* parser, const char* at, size_t length)
{
    auto http_parser = reinterpret_cast<HttpParser*>(parser->data);
    http_parser->m_field_data.m_url = std::string(at, length);
    return 0;
}

int HttpParser::OnParseStatus(llhttp_t* parser, const char* at, size_t length)
{
    auto http_parser = reinterpret_cast<HttpParser*>(parser->data);
    http_parser->m_field_data.m_status = std::string(at, length);
    return 0;
}

int HttpParser::OnParseHeaderField(llhttp_t* parser, const char* at, size_t length)
{
    auto http_parser = reinterpret_cast<HttpParser*>(parser->data);
    std::string key(at, length);
    http_parser->m_field_data.m_kv_http_response[key] = "";
    http_parser->m_last_key = key;
    return 0;
}

int HttpParser::OnParseHeaderValue(llhttp_t* parser, const char* at, size_t length)
{
    auto http_parser = reinterpret_cast<HttpParser*>(parser->data);
    std::string value(at, length);
    auto it = http_parser->m_field_data.m_kv_http_response.find(http_parser->m_last_key);
    if (it != http_parser->m_field_data.m_kv_http_response.end()) {
        it->second = value;
    } else {
        http_parser->m_field_data.m_kv_http_response[http_parser->m_last_key] = value;
    }
    return 0;
}

int HttpParser::OnParseHeadersComplete(llhttp_t* parser)
{
    // auto http_parser = reinterpret_cast<HttpParser*>(parser->data);
    // http_parser->m_is_completed = true;
    return 0;
}

int HttpParser::OnParseBody(llhttp_t* parser, const char* at, size_t length)
{
    auto http_parser = reinterpret_cast<HttpParser*>(parser->data);
    http_parser->m_field_data.m_body.append(at, length);
    return 0;
}

int HttpParser::OnParseMessageComplete(llhttp_t* parser)
{
    auto http_parser = reinterpret_cast<HttpParser*>(parser->data);
    http_parser->m_is_completed = true;
    return 0;
}

bool HttpParser::GetHeaderValue(const std::string& key, std::string& value) const
{
    auto it = m_field_data.m_kv_http_response.find(key);
    if (it != m_field_data.m_kv_http_response.end()) {
        value = it->second;
        return true;
    }
    return false;
}

const std::string& HttpParser::GetUrl() const
{
    return m_field_data.m_url;
}

const std::string& HttpParser::GetStatus() const
{
    return m_field_data.m_status;
}

const std::string& HttpParser::GetBody() const
{
    return m_field_data.m_body;
}

const std::unordered_map<std::string, std::string>& HttpParser::GetHeaders() const
{
    return m_field_data.m_kv_http_response;
}



} // namespace bbt::http::detail