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

    /**
     * @brief 获取所有请求头信息
     * 
     * @return const std::unordered_map<std::string, std::string>& 
     */
    const std::unordered_map<std::string, std::string>& GetHeaders() const;

    /**
     * @brief 获取请求Url
     * 
     * @return const std::string& 
     */
    const std::string&          GetUrl() const;

    /**
     * @brief 获取请求体
     * 
     * @return const std::string& 
     */
    const std::string&          GetBody() const;

    /**
     * @brief 获取请求的方法
     * 
     * @return const emHttpMethod& 
     */
    const emHttpMethod&         GetMethod() const;

    /**
     * @brief 根据key获取header的值
     * 
     * @param key 
     * @return const std::string& 
     */
    const std::string&          GetHeader(const std::string& key) const;

    /**
     * @brief 根据query获取请求参数
     * 
     * @param query 
     * @return const std::string& 
     */
    const std::string&          GetParam(const std::string& query) const;

    /**
     * @brief 向Response中追加一个Header
     * 
     * @param key 
     * @param value 
     * @return Context& 
     */
    Context&                    AddHeaderL(const char* key, const char* value);
    
    /**
     * @brief 异步发送一个Response（线程安全）
     * 
     * @param code 
     * @param status 
     * @param body 
     * @return core::errcode::ErrOpt 
     */
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

    ParserData              m_request_fields_data;
    static const std::string s_empty_header;
};

} // namespace bbt::http::detail