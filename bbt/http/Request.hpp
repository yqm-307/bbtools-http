#pragma once
#include <atomic>
#include <llhttp.h> // http parser
#include <bbt/http/detail/Define.hpp>
#include <bbt/core/buffer/Buffer.hpp>

namespace bbt::http
{

class Request
{
public:
    Request();
    ~Request();

    CURL*                       GetCURL();
    bool                        IsCompleted();
    void                        Clear();
    const bbt::core::Buffer&    GetRawResponse() const;

    core::errcode::ErrOpt       SetOpt(emHttpOpt opt, ...);
    void                        SetResponseCallback(const ResponseCallback& cb);
    core::errcode::ErrTuple<std::shared_ptr<detail::HttpParser>>
                                Parse() const;

private: // friend
    friend size_t OnRecvResponse(void* buf, size_t size, size_t nmemb, void* arg);
    friend class HttpClient;
    friend class detail::Context;

    void                        OnRecvResponse(const char* header, size_t len);
    void                        OnComplete(core::errcode::ErrOpt err);

private:
    CURL* m_curl{nullptr};
    // 目前简单处理，copy接收所有数据
    bbt::core::Buffer m_response_data{8};

    ResponseCallback  m_on_resp_cb{nullptr};
    std::atomic_bool  m_is_completed{false};
};

} // namespace bbt::http