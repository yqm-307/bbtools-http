#pragma once
#include <llhttp.h>
#include <bbt/http/detail/Define.hpp>

namespace bbt::http::detail
{

struct ParserData
{
    int code{-1};
    std::string status{""};
    std::string url{""};
    emHttpMethod method;
    std::string body{""};
    std::unordered_map<std::string, std::string> headers;
    std::unordered_map<std::string, std::string> parammap;
};

class HttpParser
{
    friend class Context;
public:
    explicit HttpParser(llhttp_type type);
    ~HttpParser();

    bool GetHeaderValue(const std::string& key, std::string& value) const;
    bool IsCompleted() const { return m_is_completed; }
    const std::string& GetUrl() const;
    const std::string& GetStatus() const;
    const std::string& GetBody() const;
    const std::unordered_map<std::string, std::string>& GetHeaders() const;

    core::errcode::ErrOpt ExecuteParse(const char* data, size_t len);

private:
    core::errcode::ErrOpt _PreExecuteParse();
    core::errcode::ErrOpt _DoExecuteParse(const char* data, size_t len);
    core::errcode::ErrOpt _FinishExecuteParse();

    static int OnParseUrl(llhttp_t* parser, const char* at, size_t length);
    static int OnParseStatus(llhttp_t* parser, const char* at, size_t length);
    static int OnParseHeaderField(llhttp_t* parser, const char* at, size_t length);
    static int OnParseHeaderValue(llhttp_t* parser, const char* at, size_t length);
    static int OnParseHeadersComplete(llhttp_t* parser);
    static int OnParseBody(llhttp_t* parser, const char* at, size_t length);
    static int OnParseMessageComplete(llhttp_t* parser);
private:
    llhttp_t* m_response_parser{nullptr};
    llhttp_settings_t m_settings;

    ParserData  m_response_data;

    std::string m_last_key{""};
    bool m_is_completed{false};
};

} // namespace bbt::http