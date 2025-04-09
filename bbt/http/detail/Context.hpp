#pragma once
#include <bbt/http/detail/Define.hpp>
#include <bbt/http/detail/HttpParser.hpp>


namespace bbt::http::detail
{

class Context:
    public std::enable_shared_from_this<Context>
{
public:
    Context(std::weak_ptr<HttpServer> server, evhttp_request* req);
    ~Context();

    evhttp_request*             GetRawReq() const;
    const FieldData&            GetRequestFieldData() const;

    Context&                    AddHeaderL(const char* key, const char* value);
    // 异步发送响应
    core::errcode::ErrOpt       SendReply(int code, const std::string& status, const core::Buffer& body);
private:
    friend class http::HttpServer;
    core::errcode::ErrOpt       _DoSendReply(evhttp* http);
    void                        _ParseRequest();
private:
    std::weak_ptr<HttpServer> m_server;
    int                     m_code{-1};
    std::string             m_status;

    evhttp_request*         m_req{nullptr};
    evkeyvalq*              m_headers{nullptr};
    evbuffer*               m_body{nullptr};

    core::errcode::ErrOpt   m_link_call_err{std::nullopt};
    bool                    m_is_complete{false};

    FieldData               m_request_fields_data;
};

} // namespace bbt::http::detail