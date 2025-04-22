#include <iostream>
#include <string>
#include <sstream>
#include <event2/keyvalq_struct.h>
#include <bbt/http/detail/Context.hpp>
#include <bbt/http/HttpServer.hpp>
#include <bbt/http/Request.hpp>

#define ERR_PREFIX BBT_HTTP_MODULE_NAME "[Response]"

using namespace bbt::core::errcode;

namespace bbt::http::detail
{

const std::string Context::s_empty_header = "";

Context::Context(std::weak_ptr<HttpServer> server, evhttp_request* req):
    m_server(server),
    m_req(req),
    m_headers(evhttp_request_get_output_headers(req))
{
    _ParseRequest();
}

Context::~Context()
{
    // if (m_req != nullptr)
        // evhttp_request_free(m_req);
}

const std::unordered_map<std::string, std::string>& Context::GetHeaders() const
{
    return m_request_fields_data.headers;
}

const std::string& Context::GetUrl() const
{
    return m_request_fields_data.url;
}

const std::string& Context::GetBody() const
{
    return m_request_fields_data.body;
}

const emHttpMethod& Context::GetMethod() const
{
    return m_request_fields_data.method;
}

const std::string& Context::GetHeader(const std::string& key) const
{
    auto it = m_request_fields_data.headers.find(key);
    if (it != m_request_fields_data.headers.end()) {
        return it->second;
    }
    return s_empty_header;
}

const std::string& Context::GetParam(const std::string& query) const
{
    auto it = m_request_fields_data.parammap.find(query);
    if (it != m_request_fields_data.parammap.end()) {
        return it->second;
    }
    return s_empty_header;
}


Context& Context::AddHeaderL(const char* key, const char* value)
{
    do{
        if (m_link_call_err != std::nullopt)
            break;
    
        if (m_is_complete) {
            m_link_call_err = Errcode(ERR_PREFIX "response is already complete!", emErr::ERR_UNKNOWN);
            break;
        }
    
        if (0 != evhttp_add_header(m_headers, key, value)) {
            m_link_call_err = Errcode(ERR_PREFIX "add header failed!", emErr::ERR_UNKNOWN);
            break;
        }

    } while(0);

    return *this;
}

core::errcode::ErrOpt Context::SendReply(int code, const std::string& status, const core::Buffer& body)
{
    if (m_link_call_err != std::nullopt)
        return m_link_call_err;

    if (m_req == nullptr)
        return Errcode(ERR_PREFIX "request is null!", emErr::ERR_UNKNOWN);

    auto server = m_server.lock();
    if (server == nullptr)
        return Errcode(ERR_PREFIX "httpserver is null!", emErr::ERR_UNKNOWN);

    m_body = evhttp_request_get_output_buffer(m_req);
    if (m_body == nullptr) {
        return Errcode(ERR_PREFIX "get output buffer failed!", emErr::ERR_UNKNOWN);
    }

    m_code = code;
    m_status = status;

    if (evbuffer_add(m_body, body.Peek(), body.Size()) != 0) {
        return Errcode(ERR_PREFIX "add output buffer failed!", emErr::ERR_UNKNOWN);
    }

    return server->AsyncReply(shared_from_this());
}

core::errcode::ErrOpt Context::_DoSendReply(evhttp* http)
{
    if (m_is_complete)
        return Errcode(ERR_PREFIX "response is already complete!", emErr::ERR_UNKNOWN);

    evhttp_send_reply(m_req, m_code, m_status.c_str(), evhttp_request_get_output_buffer(m_req));
    m_is_complete = true;
    return std::nullopt;
}

void Context::_ParseRequest()
{
    // 解析uri
    auto* parsed_uri = evhttp_request_get_evhttp_uri(m_req);
    m_request_fields_data.url = std::string(evhttp_uri_get_path(parsed_uri));
    auto* headers = evhttp_request_get_input_headers(m_req);
    for (auto* header = headers->tqh_first; header != nullptr; header = header->next.tqe_next) {
        m_request_fields_data.headers[header->key] = header->value;
    }

    const char* query = evhttp_uri_get_query(parsed_uri);
    if (query != nullptr) {
        std::istringstream query_stream(query);
        std::string pair;

        while (std::getline(query_stream, pair, '&')) {
            size_t pos = pair.find('=');
            if (pos != std::string::npos) {
                std::string key = pair.substr(0, pos);
                std::string value = pair.substr(pos + 1);
                m_request_fields_data.parammap[key] = value;
            }
        }
    }

    auto* body = evhttp_request_get_input_buffer(m_req);
    size_t len = evbuffer_get_length(body);

    if (len > 0) {
        m_request_fields_data.body.resize(len);
        evbuffer_copyout(body, m_request_fields_data.body.data(), len);
    }

    m_request_fields_data.method = evhttp_request_get_command(m_req);
}


}